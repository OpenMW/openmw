/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (statement.d) is part of the Monster script language
  package.

  Monster is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module monster.compiler.statement;

import std.string;
import std.stdio;

import monster.compiler.tokenizer;
import monster.compiler.expression;
import monster.compiler.scopes;
import monster.compiler.types;
import monster.compiler.block;
import monster.compiler.variables;
import monster.compiler.states;
import monster.compiler.functions;
import monster.compiler.assembler;

import monster.vm.error;
import monster.vm.mclass;

alias Statement[] StateArray;

abstract class Statement : Block
{
  // Generate byte code from this statement
  void compile();
}

// An expression that works as a statement
class ExprStatement : Statement
{
  Expression exp;

  void parse(ref TokenArray toks)
    {
      exp = Expression.identify(toks);
      reqNext(toks, TT.Semicolon, loc);
    }

  char[] toString() { return "ExprStatement: " ~ exp.toString(); }

  void resolve(Scope sc) { exp.resolve(sc); }

  void compile() { exp.evalPop(); }
}

/* Handles:
   import type1, type2, ...; // module scope
   import exp1, exp2, ...;   // local scope (not implemented yet)

   Also handles:
   with(exp1, etc) statement;
   which is equivalent to:
   {
     import exp1, etc;
     statement;
   }
*/
class ImportStatement : Statement
{
  Type typeList[];

  MonsterClass mc;

  bool isLocal;
  Statement stm;

  this(bool local = false) { isLocal = local; }

  // TODO: Things like this should be completely unnecessary - we'll
  // fix a simpler and more concise parser-system later.
  static bool canParse(TokenArray toks)
    { return
        isNext(toks, TT.Import) ||
        isNext(toks, TT.With);
    }

  void parse(ref TokenArray toks)
    {
      void getList()
        {
          typeList = [Type.identify(toks)];
          while(isNext(toks, TT.Comma))
            typeList ~= Type.identify(toks);
        }

      if(isNext(toks, TT.Import, loc))
      {
        // form: import ImportList;
        getList();
        reqNext(toks, TT.Semicolon);
      }
      else if(isNext(toks, TT.With, loc))
      {
        // form: with(ImportList) statement
        if(!isLocal)
          fail("with(...) not allowed in class scopes", loc);

        reqNext(toks, TT.LeftParen);
        getList();
        reqNext(toks, TT.RightParen);
        stm = CodeBlock.identify(toks, false);
      }
      else assert(0);
    }

  void resolve(Scope sc)
    {
      // If a statement is present (ie. if this is a with-statement),
      // wrap the imports in a code scope so they become temporary.
      if(stm !is null)
        sc = new CodeScope(sc, "with-scope");

      // Add the imports to the scope
      foreach(type; typeList)
        {
          type.resolve(sc);

          if(type.isReplacer)
            type = type.getBase();

          if(!type.isObject)
            fail("Can only import from classes", type.loc);

          auto t = cast(ObjectType)type;
          assert(t !is null);
          mc = t.getClass(type.loc);

          sc.registerImport(new ImportHolder(mc));
        }

      // Resolve the statement if present
      if(stm !is null)
        stm.resolve(sc);
    }

  void compile()
    {
      if(stm !is null)
        stm.compile();
    }
}

// Destination for a goto statement, on the form 'label:'. This
// statement is actually a bit complicated, since it must handle jumps
// occuring both above and below the label. Seeing as the assembler
// can only associate one jump with each label for jumps occuring
// before the label, we must insert multiple labels into the assembler
// if there are multiple such jumps / gotos. Therefore we must keep a
// list of jumps occuring before the label itself has been
// compiled.

// LabelStatements are also used for internal labels. They are
// inserted by loops to handle break and continue, since these have
// similar forward reference problems.
class LabelStatement : Statement
{
  // Jump indices accumulated before compile() is called
  int[] preIndex;

  // This array is empty when the label is inserted with
  // insertLabel. The label might be registered into the labels list
  // before this happens, though, if a goto statement looks up a label
  // that is not yet registered. In that case, the list below will
  // contain all the goto statements that inserted this label.
  GotoStatement gotos[];

  // Label index used after compile() is called
  int postIndex;

  // Has compiled() been called yet?
  bool isCompiled = false;

  // Permanent data for state labels.
  StateLabel *lb;

  // Stack level where this label is defined. Should be zero for state
  // labels, but might be nonzero when LabelStatements are used
  // internally in loops.
  int stacklevel;

  // Normal creation, we are parsed.
  this()
    {
      lb = new StateLabel;
      lb.ls = this;
    }

  // Forward reference. A goto statement requested a label by this
  // name, but none has been found yet.
  this(GotoStatement gs)
    {
      this();

      lb.name = gs.labelName;
      gotos ~= gs;
    }

  // Used for internal levels (like continue and break labels). The
  // parameter gives the stack level (within the function) at the
  // label location, corresponds to sc.getTotLocals(). Internal labels
  // are neither parsed nor resolved, just compiled.
  this(int stack)
    {
      this();

      stacklevel = stack;
      lb.index = -1;
    }

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Identifier) && isNext(toks, TT.Colon);
    }

  void parse(ref TokenArray toks)
    {
      // canParse has already checked the token types, all we need to
      // do is to fetch the name.
      reqNext(toks, TT.Identifier, lb.name);
      reqNext(toks, TT.Colon);

      loc = lb.name.loc;
    }

  void resolve(Scope sc)
    {
      if(!sc.isStateCode)
        fail("Labels are only allowed in state code", lb.name.loc);
      if(sc.isInLoop)
        fail("Labels are not allowed in loops", lb.name.loc);

      // Do the magic. Just tell the scope that we exist and let it
      // handle the rest.
      auto scp = sc.getState().sc;
      assert(scp !is null);
      scp.insertLabel(lb);

      stacklevel = sc.getTotLocals();
      assert(stacklevel == 0);
    }

  // Compile the label code. Note that when LabelStatement are used
  // internally (such as in for loops to handle break and continue),
  // then parse() and resolve() are never called, only compile().
  void compile()
    {
      // Since labels are also used internally, there's a chance that
      // we end up calling this function several times by mistake.
      assert(!isCompiled, "LabelStatement.compile() was called twice.");

      setLine();

      // We must insert one label for each of the jumps that occured
      // before us
      foreach(int ind; preIndex)
        tasm.labelNum(ind, lb.index);
      preIndex = null;

      // Now get a label index for all the jumps that occur after
      // us. We only need one.
      postIndex = tasm.labelNum(lb.index);

      // Make sure jump() knows what it is doing.
      isCompiled = true; 
    }

  // This is called from GotoStatement.compile() and tells us to
  // insert a jump that jumps to this label. The parameter gives the
  // stack level at the jump point.
  void jump(int stack)
    {
      // Before jumping, we have set the stack to the correct level
      // for where the label is.
      stack -= stacklevel;
      assert(stack >= 0, "Negative stack correction on jump");
      if(stack) tasm.pop(stack);

      if(isCompiled)
        // We have already compiled this label, so we have an index to
        // jump to.
        tasm.jump(postIndex);
      else
        // We must get a jump index that we can use later.
        preIndex ~= tasm.jump();
    }
}

// Used by GotoStatement and StateStatement, since both refer to a
// labels.
interface LabelUser
{
  void setLabel(LabelStatement ls);
}

// goto label; Only allowed in state code (for now)
class GotoStatement : Statement, LabelUser
{
  // Set in resolve(). This is the label we are jumping to.
  LabelStatement label;

  Token labelName;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Goto);
    }

  // Set the label
  void setLabel(LabelStatement ls)
    {
      label = ls;
      if(label is null)
        fail("Cannot find label '" ~ labelName.str ~ "'", labelName.loc);
    }

  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Goto, loc);

      // Read the label name
      if(!isNext(toks, TT.Identifier, labelName))
	fail("goto expected label identifier", toks);

      reqNext(toks, TT.Semicolon);
    }

  void resolve(Scope sc)
    {
      if(!sc.isStateCode)
        fail("Goto statements are only allowed in state code", labelName.loc);

      // Tell the state scope that we are trying to goto. The system
      // will will set our label variable for us, either straight away
      // (if the label is known) or later (if the label is a forward
      // reference.)
      sc.getState().registerGoto(labelName.str, this);
    }

  void compile()
    {
      assert(label !is null);

      // We let LabelStatement do all the dirty work.
      label.jump(0);
    }
}

// Handles continue and break, with or without a label.
class ContinueBreakStatement : Statement
{
  Token labelName;

  // Label we are jumping to. It is obtained from the scope. The label
  // we are pointing to will be a break or continue label created
  // internally in the loop coresponding to labelName. If labelName is
  // empty then the scope will use the innermost loop.
  LabelStatement label;

  // Is this a break or a continue?
  bool isBreak;

  // Used for error messages. Contains either "break" or "continue".
  char[] errName;

  // Stack level (sc.getTotLocals) at the jump point.
  int stacklevel;

  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.Continue) || isNext(toks, TT.Break); }

  void parse(ref TokenArray toks)
    {
      if(isNext(toks, TT.Continue, loc)) isBreak = false;
      else if(isNext(toks, TT.Break, loc)) isBreak = true;
      else assert(0, "Internal error");

      if(isBreak) errName = "break";
      else errName = "continue";

      if(!isNext(toks, TT.Semicolon))
        {
          if(!isNext(toks, TT.Identifier, labelName))
            fail(errName ~ " expected ; or label", toks);

          if(!isNext(toks, TT.Semicolon))
            fail(errName ~ " expected ;", toks);
        }
    }

  void resolve(Scope sc)
    {
      if(!sc.isInLoop())
        fail("Cannot use " ~ errName ~ " outside a loop", loc);

      // Get the correct label to jump to
      if(isBreak) label = sc.getBreak(labelName.str);
      else label = sc.getContinue(labelName.str);

      // And the stack level at the jump point
      stacklevel = sc.getTotLocals();

      if(label is null)
        {
          assert(labelName.str != "", "Internal error");
          fail("Loop label '" ~ labelName.str ~ "' not found", labelName.loc);
        }
    }

  void compile()
    {
      // Our nice LabelStatement implementation does everything for
      // us.
      assert(label !is null);
      label.jump(stacklevel);
    }
}

// do-while loop  (might also support do-until loops in the future)
// do statement while (expression)
// do : label statement while (expression)
class DoWhileStatement : Statement
{
  Expression condition;
  Statement block;

  Token labelName;

  LoopScope sc;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Do);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.Do, loc))
	assert(0, "Internal error");

      // Is there a loop label?
      if(isNext(toks, TT.Colon))
        if(!isNext(toks, TT.Identifier, labelName))
          fail("do statement expected label identifier", toks);

      // Read the statement ('false' to disallow variable
      // declarations.)
      block = CodeBlock.identify(toks, false);

      if(!isNext(toks, TT.While))
	fail("do statement expected while(...)", toks);

      if(!isNext(toks, TT.LeftParen))
	fail("while statement expected '('", toks);

      // Parse the conditional expression
      condition = Expression.identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("while statement expected ')'", toks);

      // Allow an optional semicolon after the while()
      isNext(toks, TT.Semicolon);
    }

  void resolve(Scope last)
    {
      sc = new LoopScope(last, this);

      // Resolve all parts
      condition.resolve(sc);

      if(!condition.type.isBool)
	fail("while condition " ~ condition.toString ~ " must be a bool, not "
	     ~condition.type.toString, condition.loc);

      block.resolve(sc);
    }

  void compile()
    {
      LabelStatement
        cont = sc.getContinue(),
        brk = sc.getBreak();

      setLine();

      int label = tasm.label();

      // Execute the block
      block.compile();

      // Continue label goes here, right after the code block
      cont.compile();

      // Push the conditionel expression
      condition.eval();

      setLine();

      // Jump if the value is non-zero.
      tasm.jumpnz(label);

      // Break label go here, after the entire loop
      brk.compile();
    }
}

// while loop:
// while (expression) statement
// while (expression) : label statement
class WhileStatement : Statement
{
  Expression condition;
  Statement block;

  Token labelName;

  LoopScope sc;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.While);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.While, loc))
	assert(0);

      if(!isNext(toks, TT.LeftParen))
	fail("while statement expected '('", toks);

      // Parse the conditional expression
      condition = Expression.identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("while statement expected ')'", toks);

      // Is there a loop label?
      if(isNext(toks, TT.Colon))
        if(!isNext(toks, TT.Identifier, labelName))
          fail("while statement expected label identifier", toks);

      // Read the statement ('false' to disallow variable
      // declarations.)
      block = CodeBlock.identify(toks, false);
    }

  void resolve(Scope sco)
    {
      sc = new LoopScope(sco, this);

      // Resolve all parts
      condition.resolve(sc);

      if(!condition.type.isBool)
	fail("while condition " ~ condition.toString ~ " must be a bool, not "
	     ~condition.typeString, condition.loc);

      block.resolve(sc);
    }

  void compile()
    {
      /* To avoid the extra jump back and forth at the end, the coding
	 here is similar to
	 if(condition)
	 {
	   do
             block
           while(condition);
	 }
      */

      LabelStatement
        cont = sc.getContinue(),
        brk = sc.getBreak();

      // Evaluate the condition for the first iteration
      condition.eval();

      setLine();

      // Skip the entire loop if the condition is not met
      int outer = tasm.jumpz();

      // Jump here to repeat the loop
      int inner = tasm.label();

      // Execute the block
      block.compile();

      // Continue label
      cont.compile();

      // Push the conditionel expression
      condition.eval();

      setLine();

      // Repeat the loop if the value is non-zero.
      tasm.jumpnz(inner);

      tasm.label(outer);

      // Break label
      brk.compile;
    }
}

/* foreach loop:
   foreach(indexDeclaration; arrayExpr) statement
   foreach(indexDeclaration; arrayExpr) : label statement

   arrayExpr - any expression that evaluates to an array.

   indexDeclaration - either:

     type var

   or

     int index, type var

   where 'type' is the base type of arrayExpr. Either or both types
   may be omitted since they can always be infered by the
   compiler. For example:

   foreach(i,v; "hello") // i is type int, v is type char

   Using 'ref' in the declaration of the value variable means that
   changes to it will affect the original array.

   Use foreach_reverse instead of foreach to iterate the array in the
   opposite direction.
 */
class ForeachStatement : Statement
{
  Expression arrayExp;
  VarDeclaration index, value;
  Statement block;

  LoopScope sc;
  Token labelName;
  bool isReverse; // Traverse in reverse order
  bool isRef; // Set if the value variable is a reference (alters
              // original content)

  // Set if we are traversing the objects of a class
  Token className;
  bool isClass = false;
  MonsterClass clsInfo;

  char[] toString()
    {
      char[] res = "foreach";
      if(isReverse) res ~= "_reverse";
      res ~= "(";

      if(index !is null) res ~= index.toString ~ ", ";
      res ~= value.toString ~ "; " ~ arrayExp.toString ~ ")";

      if(labelName.str != "") res ~= " : " ~ labelName.str;

      res ~= "\n" ~ block.toString;

      return res;
    }

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Foreach) || isNext(toks, TT.ForeachRev);
    }

  void parse(ref TokenArray toks)
    {
      if(isNext(toks, TT.Foreach, loc)) isReverse = false;
      else if(isNext(toks, TT.ForeachRev, loc)) isReverse = true;
      else assert(0);

      if(!isNext(toks, TT.LeftParen))
        fail("foreach statement expected '('", toks);

      // Read the first variable declaration (TODO: allow
      // 'ref'). Assume it is the value.
      value = new VarDeclaration();
      value.allowRef = true;
      value.allowNoType = true;
      value.parse(toks);

      if(value.init !is null)
        fail("Variable initializer is not allowed in foreach", loc);

      if(isNext(toks, TT.Comma)) // Is there another one?
        {
          // The previous variable was the index, not the value
          index = value;

          // Sanity check the index variable. TODO: At some point it
          // might be possible to mark it as constant.
          if(index.var.isRef)
            fail("Index cannot be a reference variable", loc);

          value = new VarDeclaration();
          value.allowRef = true;
          value.allowNoType = true;
          value.parse(toks);
          if(value.init !is null)
            fail("Variable initializer is not allowed in foreach", loc);
        }

      isRef = value.var.isRef;

      if(!isNext(toks, TT.Semicolon))
        fail("foreach expected ;", toks);

      // Is the expression a class?
      if(isNext(toks, TT.Class))
        {
          // Get the class name
          if(!isNext(toks, TT.Identifier, className))
            fail("foreach expected class name after 'class'");
          isClass = true;
        }
      else
        {
          // Get the array
          arrayExp = Expression.identify(toks);
        }

      if(!isNext(toks, TT.RightParen))
        fail("foreach statement expected ')'", toks);

      // Is there a loop label?
      if(isNext(toks, TT.Colon))
        if(!isNext(toks, TT.Identifier, labelName))
          fail("foreach statement expected label identifier", toks);

      // Read the statement (the 'false' parameter is to disallow
      // variable declarations.)
      block = CodeBlock.identify(toks, false);
    }

  void resolve(Scope sco)
    {
      if(sco.isStateCode)
        fail("Foreach loops are currently not allowed in state code.");

      sc = new LoopScope(sco, this);

      if(isClass)
        {
          // Class loops

          clsInfo = global.findClass(className);
          assert(clsInfo !is null);

          if(index !is null)
            fail("Index not allowed in class iteration");

          // Set the value variable type if it is missing
          if(value.var.type is null)
            value.var.type = clsInfo.objType;

          // Resolve and allocate stack for the value variable
          value.resolve(sc);

          // Check that the type is correct
          if(value.var.type.toString != className.str)
            fail("Loop variable must be of type " ~ className.str ~ ", not "
                 ~ value.var.type.toString, value.var.name.loc);

          // Reference variables are not allowed
          if(value.var.isRef)
            fail("Reference variable not allowed in class iteration.");

          // Reverse is not allowed
          if(isReverse)
            fail("Cannot traverse class instances in reverse order.");
        }
      else
        {
          // This is for array loops

          // Resolve the index, if present
          if(index !is null)
            {
              if(index.var.type is null)
                index.var.type = BasicType.getInt;

              index.resolve(sc);

              if(!index.var.type.isInt)
                fail("foreach index type must be an int, not " ~
                     index.var.type.toString, index.var.name.loc);
            }
          // If not, allocate a stack value for it anyway.
          else sc.addNewVar(1);

          // Check that the array is in fact an array
          arrayExp.resolve(sc);
          if(!arrayExp.type.isArray)
            fail("foreach expected an array, not " ~ arrayExp.toString, arrayExp.loc);

          if(value.var.type is null)
            value.var.type = arrayExp.type.getBase();

          // This also allocates a stack value
          value.resolve(sc);

          if(value.var.type != arrayExp.type.getBase())
            fail("foreach iteration variable must be of type " ~
                 arrayExp.type.getBase().toString() ~ ", not " ~
                 value.var.type.toString(), value.var.name.loc);
        }

      // Tell the scope that the iterator index is on the stack, like
      // a local variable. This is not the same as the index variable
      // above - the iterator index is an internal variable used by
      // the system for storing the iterator state.
      sc.addNewVar(1);

      // This defines the stack level of the loop internals. Break and
      // continue labels are set up so that they return to this level
      // when invoked.
      sc.stackPoint();

      // Our stack now looks like this (last pushed at the top):

      // For arrays:
      // Array Index / iterator index
      // Value variable
      // Index variable

      // For classes:
      // Iterator index
      // Object index variable

      // It is important that we have called sc.addNewLocalVar() for
      // each of these, to let the scope know about our stack
      // usage. Otherwise local variables declared inside the loop
      // body will not work correctly. This is either done explicitly
      // or through VarDecl.resolve().

      block.resolve(sc);
    }

  void compile()
    {
      // Get loop labels. These are set up after the loop variables
      // and the array index was declared, and their stack levels
      // reflect that. They must only be compiled while the variables
      // are still on the stack.
      LabelStatement
        cont = sc.getContinue(),
        brk = sc.getBreak();

      setLine;

      // First push the index, in case of array iteration
      if(!isClass)
        {
          // Push the variables on the stack
          if(index !is null) index.compile();
          else tasm.push(0); // Create an unused int on the stack
        }

      // Then push the value
      value.compile();

      // Create iteration reference.
      if(isClass)
        {
          // Classes. Create a class iterator from the given class
          // number.
          tasm.createClassIterator(clsInfo.getIndex());
        }
      else
        {
          // Arrays. First, evaluate the array expression to get the
          // array index on the stack.
          arrayExp.eval();

          // This will replace the array index with an "iterator"
          // index. The VM will handle the iteration data from there,
          // it is "aware" of the index and value on the stack as
          // well.
          tasm.createArrayIterator(isReverse, isRef);
        }

      setLine();

      // The rest is the same for all iterators.

      // Skip the loop the array is empty
      int outer = tasm.jumpz();

      // Jump here to repeat the loop
      int inner = tasm.label();

      // Execute the block
      block.compile();

      // Continue statements bring us here
      cont.compile();

      setLine;

      // Go to next iteration. Leaves the iteration index and
      // variables on the stack. Pushes true if we should continue, or
      // false otherwise. The iterator reference is destroyed when the
      // last iteration is done.
      tasm.iterateNext();

      // Repeat the loop if the value is non-zero.
      tasm.jumpnz(inner);
      int outer2 = tasm.jump();

      // Break statements get us here. We have to finish the last
      // iteration step (copy ref values and delete the
      // iterator). This code is NOT guaranteed to be run. If we
      // break/continue to an outer loop, or return from the function,
      // this will not be run and the iterator never destroyed. This
      // is not a big problem though, since the iterator list is
      // cleared by the garbage collector.
      brk.compile();

      setLine;

      tasm.iterateBreak();

      tasm.label(outer);
      tasm.label(outer2);

      // Undo the local scope
      tasm.pop(sc.getLocals);
    }
}


/* for loop:
   for(initExpr; condExpr; iterExpr) statement
   for(initExpr; condExpr; iterExpr) : label statement

   initExpr is either an expression (of any type), a variable
   declaration (scoped only for the interior of the loop), or empty

   condExpr is an expression of type bool, or empty

   iterExpr is an expression of any type, or empty

   An empty condExpr is treated as true. The label, if specified, may
   be used with break and continue statements inside the loop.
*/
class ForStatement : Statement
{
  Expression init, condition, iter;
  VarDeclStatement varDec;

  Token labelName;

  Statement block;

  LoopScope sc; // Scope for this loop

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.For);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.For, loc))
	assert(0);

      if(!isNext(toks, TT.LeftParen))
	fail("for statement expected '('", toks);

      // Check if the init as a variable declaration, if so then parse
      // it as a statement (since that takes care of multiple
      // variables as well.)
      if(VarDeclStatement.canParse(toks))
	{
	  varDec = new VarDeclStatement;
	  varDec.parse(toks); // This also kills the trailing ;
	}
      // Is it an empty init statement?
      else if(!isNext(toks, TT.Semicolon))
	{
	  // If not, then assume it's an expression
	  init = Expression.identify(toks);
	  if(!isNext(toks, TT.Semicolon))
	    fail("initialization expression " ~ init.toString() ~
		 " must be followed by a ;", toks);
	}

      // Phew! Now read the conditional expression, if there is one
      if(!isNext(toks, TT.Semicolon))
	{
	  condition = Expression.identify(toks);
	  if(!isNext(toks, TT.Semicolon))
	    fail("conditional expression " ~ condition.toString() ~
		 " must be followed by a ;", toks);
	}

      // Finally the last expression
      if(!isNext(toks, TT.RightParen))
	{
	  iter = Expression.identify(toks);

	  if(!isNext(toks, TT.RightParen))
	    fail("for statement expected ')'", toks);
	}

      // Is there a loop label?
      if(isNext(toks, TT.Colon))
        if(!isNext(toks, TT.Identifier, labelName))
          fail("for statement expected label identifier", toks);

      // Read the statement (the 'false' parameter is to disallow
      // variable declarations.)
      block = CodeBlock.identify(toks, false);
    }

  void resolve(Scope sco)
    {
      sc = new LoopScope(sco, this);

      // Resolve all parts
      if(init !is null)
	{
	  init.resolve(sc);
	  assert(varDec is null);
	}
      if(varDec !is null)
	{
	  varDec.resolve(sc);
	  assert(init is null);
	}

      // This affects break and continue, making sure they set the
      // stack properly when jumping.
      sc.stackPoint();

      if(condition !is null)
	{
	  condition.resolve(sc);
	  if(!condition.type.isBool)
	    fail("for condition " ~ condition.toString ~ " must be a bool, not "
		 ~condition.type.toString, condition.loc);
	}
      if(iter !is null) iter.resolve(sc);

      block.resolve(sc);
    }

  void compile()
    {
      // Compiled in a similar way as a while loop, with an outer
      // check for the initial state of the condition that is separate
      // from the check after each iteration.

      // Get loop labels. Remember that these are set up after the
      // loop variable was declared (if any), and their stack levels
      // reflect that. They must only be compiled while the variable
      // is still on the stack.
      LabelStatement
        cont = sc.getContinue(),
        brk = sc.getBreak();

      // Push any local variables on the stack, or do initialization.
      if(varDec !is null) varDec.compile();
      else if(init !is null)
        init.evalPop();

      int outer;

      // Evaluate the condition for the first iteration
      if(condition !is null)
	{
	  condition.eval();
	  // Skip the entire loop if the condition is not met
	  outer = tasm.jumpz();
	}
      else
	// A missing condition is always met, do nothing
	outer = -1;

      // Jump here to repeat the loop
      int inner = tasm.label();


      // Execute the block
      block.compile();

      // Continue statements bring us here
      cont.compile();

      // Do the iteration step, if any
      if(iter !is null) iter.evalPop();

      // Push the conditionel expression, or assume it's true if
      // missing.
      if(condition !is null)
	{
	  condition.eval();
	  // Repeat the loop if the value is non-zero.
	  tasm.jumpnz(inner);
	}
      else tasm.jump(inner);

      if(outer != -1) tasm.label(outer);

      // Break statements get us here
      brk.compile();

      // Undo the local scope
      tasm.pop(sc.getLocals);
    }
}

// Handles state = statename; Might be removed if states become a real
// type. Supports state = null; as well, to set the null state. You
// may specify a label to jump to, as state = statename.label;
class StateStatement : Statement, LabelUser
{
  Token stateName, labelName;

  State* stt;
  LabelStatement label;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.State);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.State, loc))
	assert(0, "Internal error in StateStatement");

      if(!isNext(toks, TT.Equals))
	fail("state statement expected =", toks);

      if(!isNext(toks, TT.Identifier, stateName) &&
	 !isNext(toks, TT.Null) )
	fail("state name identifier or null expected", toks);

      // Check if a label is specified
      if(stateName.type == TT.Identifier)
        if(isNext(toks, TT.Dot))
          if(!isNext(toks, TT.Identifier, labelName))
            fail("state label expected after .", toks);

      if(!isNext(toks, TT.Semicolon))
	fail("state statement expected ;", toks);
    }

  // Set the label to jump to. This is called from the state
  // declaration. The argument is null if the label was never
  // resolved.
  void setLabel(LabelStatement ls)
    {
      if(ls is null)
        fail("Undefined label '" ~ labelName.str ~ "'", labelName.loc);
      label = ls;
    }

  void resolve(Scope sc)
    {
      // Check the state name.
      if(stateName.type == TT.Identifier)
	{
          auto sl = sc.lookup(stateName);
	  if(!sl.isState)
	    fail("Undefined state " ~ stateName.str, stateName.loc);

	  stt = sl.state;

          // If a label is specified, tell the state that we are
          // asking for a label. The function will set label for us,
          // either now or when the label is resolved. The label will
          // in any case always be set before compile() is called.
          if(labelName.str != "")
            stt.registerGoto(labelName.str, this);
        }
      else
        {
          assert(labelName.str == "");
          stt = null; // Signifies the empty state
          label = null; // And no label
        }
    }

  void compile()
    {
      setLine();

      // If there is a label, make sure it has been resolved.
      assert(labelName.str == "" || label !is null);

      if(stt is null)
        {
          tasm.setState(-1, -1, 0);
          assert(label is null);
          return;
        }

      int cindex = stt.owner.getTreeIndex();

      if(label is null)
        tasm.setState(stt.index, -1, cindex);
      else
        tasm.setState(stt.index, label.lb.index, cindex);
    }
}

// Handles if(expr) {} and if(expr) {} else {}. Expr must be of type
// bool. (We will not support variable declarations in the expression,
// since the strict bool requirement would make it useless anyway.)
class IfStatement : Statement
{
  Expression condition;
  Statement block, elseBlock;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.If);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.If, loc))
	assert(0, "Internal error in IfStatement");

      if(!isNext(toks, TT.LeftParen))
	fail("if statement expected '('", toks);

      // Parse conditional expression.
      condition = Expression.identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("if statement expected ')'", toks);

      // Parse the first statement, but disallow variable
      // declarations. (if(b) int i; is not allowed, but if(b) {int
      // i;} works ok)
      block = CodeBlock.identify(toks, false);

      // We are either done now, or there is an 'else' following the
      // first statement.
      if(isNext(toks, TT.Else))
	elseBlock = CodeBlock.identify(toks, false);
    }

  void resolve(Scope sc)
    {
      // Resolve all parts
      condition.resolve(sc);

      if(!condition.type.isBool &&
         !condition.type.isArray &&
         !condition.type.isObject)
	fail("if condition " ~ condition.toString ~ " must be a bool, not "
	     ~condition.type.toString, condition.loc);

      block.resolve(sc);
      if(elseBlock !is null) elseBlock.resolve(sc);
    }

  void compile()
    {
      bool hasElse = elseBlock !is null;

      // Push the conditionel expression
      condition.eval();

      // For arrays, get the length
      if(condition.type.isArray)
        tasm.getArrayLength();

      // TODO: For objects, the null reference will automatically
      // evaluate to false. However, we should make a "isValid"
      // instruction, both to check if the object is dead (deleted)
      // and to guard against any future changes to the object index
      // type.
      assert(condition.type.getSize == 1);

      // Jump if the value is zero. Get a label reference.
      int label = tasm.jumpz();

      // Execute the first block
      block.compile();

      if(hasElse)
	{
	  // If the condition was true, we must skip the else block
	  int l2 = tasm.jump();
	  // Otherwise, we jump here to execute the else block
	  tasm.label(label);
	  elseBlock.compile();
	  tasm.label(l2);
	}
      else tasm.label(label);
    }
}

// Return statement - on the form return; or return expr;
class ReturnStatement : Statement
{
  Expression exp;
  Function *fn;

  // Number of local variables to unwind from the stack. Calculated
  // both in resolve and in compile.
  int locals;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Return);
    }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.Return, loc))
	assert(0, "Internal error in ReturnStatement");

      if(!isNext(toks, TT.Semicolon))
	{
	  exp = Expression.identify(toks);

	  if(!isNext(toks, TT.Semicolon))
	    fail("Return statement expected ;", toks);
	}
    }

  char[] toString()
    {
      if(exp is null) return "ReturnStatement;";
      return "ReturnStatement: " ~ exp.toString;
    }

  void resolve(Scope sc)
    {
      /*
      // Not allowed in state code.
      if(sc.isStateCode)
	fail("return not allowed in state code", loc);
      */
      // Return in state code is handled as a special case
      if(sc.isStateCode)
        {
          assert(!sc.isInFunc);
          if(exp !is null)
            fail("Cannot return an expression in state code", loc);
          return;
        }

      // Store the number of local variables we have to pop of the
      // stack
      locals = sc.getTotLocals;

      fn = sc.getFunction();
      assert(fn !is null, "return called outside a function scope");

      // Get the size of all parameters
      locals += fn.paramSize;

      // Next, we must check that the returned expression, if any, is
      // of the right type.
      if(exp is null)
	{
	  if(!fn.type.isVoid)
            fail(format("Function expected a return value of type '%s'",
                        fn.type.toString()), loc);
	  return;
	}

      if(fn.type.isVoid)
        fail("Function does not have a return type", loc);

      exp.resolve(sc);

      try fn.type.typeCast(exp);
      catch(TypeException)
	fail(format(
            "Function '%s' expected return type '%s', not '%s' of type '%s'",
                    fn.name.str, fn.type.toString(),
                    exp, exp.type.toString()),
             exp.getLoc);
    }

  void compile()
    {
      setLine();
      if(exp !is null && fn !is null)
	{
	  assert(!fn.type.isVoid);
	  exp.eval();
          // Return an expression
	  tasm.exit(locals, exp.type.getSize);
	}
      else
        // Return without an expression
	tasm.exit(locals);
    }
}

// A block of executable statements between a pair of curly braces {}
class CodeBlock : Statement
{
  StateArray contents;
  Scope sc;

  Floc endLine; // Last line, used in error messages

  bool isState; // True if this block belongs to a state
  bool isFile; // True if this is a stand-alone file

 private:

  char[] stateName; // Name of the state, used to set up the
                    // scope. Really a hack but it works.

  // Parses the given tokens as a statement. The second parameter
  // specifies if the statement is allowed to be a variable
  // declaration (for example, "if (expr) int i;" is not allowed.)
  static Statement identify(ref TokenArray toks, bool allowVar = true)
    {
      Statement b = null;

      if(VarDeclStatement.canParse(toks))
	{
	  if(!allowVar) fail("Variable declaration not allowed here", toks[0].loc);
	  b = new VarDeclStatement;
	}
      else if(CodeBlock.canParse(toks)) b = new CodeBlock;
      else if(ReturnStatement.canParse(toks)) b = new ReturnStatement;
      else if(IfStatement.canParse(toks)) b = new IfStatement;
      else if(DoWhileStatement.canParse(toks)) b = new DoWhileStatement;
      else if(WhileStatement.canParse(toks)) b = new WhileStatement;
      else if(ForStatement.canParse(toks)) b = new ForStatement;
      else if(StateStatement.canParse(toks)) b = new StateStatement;
      else if(LabelStatement.canParse(toks)) b = new LabelStatement;
      else if(GotoStatement.canParse(toks)) b = new GotoStatement;
      else if(ContinueBreakStatement.canParse(toks)) b = new ContinueBreakStatement;
      else if(ForeachStatement.canParse(toks)) b = new ForeachStatement;
      else if(ImportStatement.canParse(toks)) b = new ImportStatement(true);
      // switch / select
      // case
      // assert ?

      // If this is not one of the above, default to an expression
      // statement.
      else b = new ExprStatement;

      b.parse(toks);
      return b;
    }

 public:

  this(bool isState = false, bool isFile = false)
    {
      this.isState = isState;
      this.isFile = isFile;

      assert(!isFile || !isState);
    }

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.LeftCurl);
    }

  void parse(ref TokenArray toks)
    {
      Token strt;
      if(!isFile)
        {
          if(toks.length == 0)
            fail("Code block expected, got end of file");

          strt = toks[0];
          if(strt.type != TT.LeftCurl)
            fail("Code block expected a {", toks);

          loc = strt.loc;

          toks = toks[1..toks.length];
        }

      // Are we parsing stuff that comes before the code? Only
      // applicable to state blocks.
      bool beforeCode = isState;

      bool theEnd()
        {
          if(isFile)
            return isNext(toks, TT.EOF, endLine);
          else
            return isNext(toks, TT.RightCurl, endLine);
        }

      while(!theEnd())
	{
	  if(toks.length == 0)
            {
              if(!isFile)
                fail(format("Unterminated code block (starting at line %s)", strt.loc));
              else break;
            }

          if(beforeCode)
            {
              // The first label marks the begining of the code block
              if(LabelStatement.canParse(toks))
                {
                  // Let identify insert the label
                  contents ~= identify(toks);

                  // We are now parsing code
                  beforeCode = false;

                  continue;
                }
              // TODO: Handle state functions here
              else
                fail("State code must begin with a label", toks);
            }

          contents ~= identify(toks);
	}
    }

  char[] toString()
    {
      char[] res = "Codeblock: ";
      for(int i=0; i<contents.length; i++)
	res ~= "\n  " ~ contents[i].toString();
      return res;
    }

  void resolve(Scope prev)
    {
      // Set up the local scope
      sc = new CodeScope(prev, this);

      // Make sure that isStateCode is true at the main state level.
      assert(!isState || sc.isStateCode());

      foreach(int i, stats; contents)
        stats.resolve(sc);

      // TODO: Check that state code contains at least one idle
      // function call. We could do that through the scope.
    }

  void compile()
    {
      foreach(st; contents)
        st.compile();

      setLine();

      // If this is the main block at the state level, we must finish
      // it with an exit instruction.
      if(isState)
        tasm.exit();

      // Local variables are forbidden in any state block, no matter
      // at what level they are
      if(sc.isStateCode)
        assert(sc.getLocals == 0);
      else
	// Remove local variables from the stack
	tasm.pop(sc.getLocals);
    }
}

// Used for declarations that create new types, like enum, struct,
// etc.
abstract class TypeDeclaration : Statement
{
  override void compile() {}

  // Insert the type into the scope. This is called before resolve().
  void insertType(TFVScope sc);
}
