/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (variables.d) is part of the Monster script language
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

module monster.compiler.variables;

import monster.compiler.types;
import monster.compiler.tokenizer;
import monster.compiler.expression;
import monster.compiler.scopes;
import monster.compiler.statement;
import monster.compiler.block;
import monster.compiler.operators;
import monster.compiler.assembler;

import std.string;
import std.stdio;

import monster.vm.mclass;
import monster.vm.error;

enum VarType
  {
    Class,
    Param,
    Local,
  }

struct Variable
{
  Type type;
  VarType vtype;
  Token name;

  VarScope sc; // Scope that owns this variable

  int number; // Index used in bytecode to reference this variable

  bool isRef; // Is this a reference variable?
  bool isConst; // Used for function parameters
  bool isVararg; // A vararg function parameter
}

// Variable declaration. Handles local and class variables, function
// parameters and loop variables.
class VarDeclaration : Block
{
  Variable *var;

  // Initializer expression, if any. Eg:
  // int i = 3;   =>  init = 3.
  Expression init;

  bool allowRef; // Allows reference variable.
  bool allowNoType; // Allow no type to be specified (used in foreach)
  bool allowConst; // Allows const.

  this() {}

  // Used when the type is already given, and we only need to read the
  // name and what follows. This is used for multiple declarations,
  // ie.  int i=1, j=2; and will possibly also be used in other places
  // later.
  this(Type type)
    {
      assert(var is null);
      var = new Variable;

      var.type = type;
    }

  // Parse keywords allowed on variables
  private void parseKeywords(ref TokenArray toks)
    {
      Floc loc;
      while(1)
	{
	  if(isNext(toks, TT.Ref, loc))
	    {
	      if(var.isRef)
		fail("Multiple token 'ref' in variable declaration",
		     loc);
              if(!allowRef)
                fail("You cannot use 'ref' variables here", loc);
	      var.isRef = true;
	      continue;
	    }
          if(isNext(toks, TT.Const, loc))
            {
              if(var.isConst)
                fail("Multiple token 'const' in variable declaration",
                     loc);
              var.isConst = true;
              continue;
            }
	  break;
	}
    }

  // Parse a series of array specifiers, ie.
  // []
  // [][]...
  // [expr1][expr2]....
  // If takeExpr = false then the last form is not allowed
  static ExprArray getArray(ref TokenArray toks, bool takeExpr = false)
    {
      // Arrays?
      ExprArray arrayArgs;
      while(isNext(toks,TT.LeftSquare))
	{
	  Expression expr = null;

	  // Is there an expression inside the brackets?
	  if(!isNext(toks, TT.RightSquare))
	    {
	      Floc loc = getLoc(toks);

	      expr = Expression.identify(toks);

	      if(!takeExpr)
		fail("Array expression [" ~ expr.toString ~
		     "] not allowed here", loc);

	      if(!isNext(toks, TT.RightSquare))
		fail("Expected matching ]", toks);
	    }

	  // Insert the expression (or a null if the brackets were
	  // empty)
	  arrayArgs ~= expr;
	}
      return arrayArgs;
    }

  // Get the total number of array dimensions. Eg. int[] j[][]; has a
  // total of three dimensions. This is now handled entirely by the
  // Type class so this function is here for backwards compatability.
  int arrays()
  {
    return var.type.arrays;
  }

  // This is slightly messy. But what I'm trying to do IS slightly
  // messy.
  static bool hasType(TokenArray toks)
    {
      // Remove the type, if any
      if(!Type.canParseRem(toks)) return false;

      // Skip any keywords
      while(1)
        {
	  if(isNext(toks, TT.Ref)) continue;
          break;
        }

      // There must be a variable identifier at the end
      return isNext(toks, TT.Identifier);
    }

  override void parse(ref TokenArray toks)
    {
      if(var is null)
        {
          var = new Variable;

          // Keywords may come before or after the type
          parseKeywords(toks);

          // Parse the type, if any.
          if(!allowNoType || hasType(toks))
            {
              var.type = Type.identify(toks);

              parseKeywords(toks);
            }
        }
      // The type was already set externally.
      else
        {
          assert(var.type !is null);

          // allowNoType is not used in these cases
          assert(allowNoType == false);
        }

      if(!isNext(toks, TT.Identifier, var.name))
        fail("Variable name must be identifier", toks);

      loc = var.name.loc;

      // Look for arrays after the variable name.
      ExprArray arrayArgs = getArray(toks);

      /* We must append these arrays to the type, in the reverse
         order. Eg.

         int[1][2] i[4][3];

         is the same as

         int[1][2][3][4] i;

         (also the same as int i[4][3][2][1];)

         Since we don't take expressions here yet the order is
         irrelevant, but let's set it up right.
      */
      foreach_reverse(e; arrayArgs)
        {
          assert(e is null);
          var.type = new ArrayType(var.type);
        }

      // Does the variable have an initializer?
      if(isNext(toks, TT.Equals))
	init = Expression.identify(toks);
    }

  char[] toString()
    {
      char[] res = var.type.toString() ~ " " ~ var.name.str;
      if(init !is null)
	res ~= " = " ~ init.toString;
      return res;
    }

  bool isParam = false;

  // Special version only called from FuncDeclaration.resolve()
  void resolveParam(Scope sc)
    {
      isParam = true;
      resolve(sc);
    }

  // Sets var.number. Only for function parameters.
  void setNumber(int num)
    {
      assert(num<0, "VarDec.setNumber was given a positive num: " ~ .toString(num));
      assert(isParam);
      var.number = num;
    }

  // Calls resolve() for all sub-expressions
  override void resolve(Scope sc)
    {
      var.type.resolve(sc);

      if(var.type.isReplacer)
        var.type = var.type.getBase();

      // Allow 'const' for function array parameters
      if(isParam && var.type.isArray())
        allowConst = true;

      // Handle initial value normally
      if(init !is null)
        {
          init.resolve(sc);

          // If 'var' is present, just copy the type of the init value
          if(var.type.isVar)
            var.type = init.type;
          else
            {
              // Convert type, if necessary.
              try var.type.typeCast(init);
              catch(TypeException)
                fail(format("Cannot initialize %s of type %s with %s of type %s",
                            var.name.str, var.type,
                            init, init.type), loc);
            }
          assert(init.type == var.type);
        }

      // If it's a struct, check that the variable is not part of
      // itself.
      if(var.type.isStruct)
        {
          auto st = cast(StructType)var.type;
          if(st.sc is sc)
            {
              // We are inside ourselves
              assert(sc.isStruct);

              fail("Struct variables cannot be declared inside the struct itself!",
                   loc);
            }
        }

      // We can't have var at this point
      if(var.type.isVar)
        fail("cannot implicitly determine type", loc);

      // Illegal types are illegal
      if(!var.type.isLegal)
        fail("Cannot create variables of type " ~ var.type.toString, loc);

      if(!allowConst && var.isConst)
        fail("'const' is not allowed here", loc);

      // Store the scope in the var struct for later referral.
      var.sc = cast(VarScope)sc;
      assert(var.sc !is null, "variables can only be declared in VarScopes");

      if(!isParam)
        // If we are not a function parameter, we must get
        // var.number from the scope.
        var.number = sc.addNewVar(var.type.getSize());
      else
        assert(sc.isFunc());

      // Insert ourselves into the scope.
      var.sc.insertVar(var);
    }

  int[] getCTimeValue()
    out(res)
    {
      assert(res.length == var.type.getSize, "Size mismatch");
    }
    body
    {
      // Does this variable have an initializer?
      if(init !is null)
        {
          // And can it be evaluated at compile time?
          if(!init.isCTime)
            fail("Expression " ~ init.toString ~
                 " is not computable at compile time", init.loc);

          return init.evalCTime();
        }
      else
        // Use the default initializer.
        return var.type.defaultInit();
    }

  // Executed for local variables upon declaration. Push the variable
  // on the stack.
  void compile()
    {
      // Validate the type
      var.type.validate(loc);

      setLine();

      if(init !is null)
        // Push the initializer
	init.eval();
      else
        // Default initializer
        var.type.pushInit();
    }
}

// Represents a reference to a variable. Simply stores the token
// representing the identifier. Evaluation is handled by the variable
// declaration itself. This allows us to use this class for local and
// global variables as well as for properties, without handling each
// case separately. The special names (currently __STACK__) are
// handled internally.
class VariableExpr : MemberExpression
{
  Token name;

  ScopeLookup look;

  // Used to simulate a member for imported variables
  DotOperator dotImport;
  bool recurse = true;

  enum VType
    {
      None,             // Should never be set
      LocalVar,         // Local variable
      ThisVar,          // Variable in this object and this class
      ParentVar,        // Variable in another class but this object
      FarOtherVar,      // Another class, another object
      Property,         // Property (like .length of arrays)
      Special,          // Special name (like __STACK__)
      Type,             // Typename
    }

  VType vtype;

  CIndex singCls = -1;  // Singleton class index

  static bool canParse(TokenArray toks)
    {
      return
        isNext(toks, TT.Identifier) ||
        isNext(toks, TT.Singleton) ||
        isNext(toks, TT.State) ||
        isNext(toks, TT.Clone) ||
        isNext(toks, TT.Const);
    }

  // Does this variable name refer to a type name rather than an
  // actual variable? TODO: Could be swapped for "requireStatic" or
  // similar in expression instead.
  bool isType() { return type.isMeta(); }

  bool isProperty()
  out(res)
  {
    if(res)
      {
        assert(vtype == VType.Property);
        assert(!isSpecial);
      }
    else assert(vtype != VType.Property);
  }
  body
  {
    return look.isProperty();
  }

  bool isSpecial() { return vtype == VType.Special; }

 override:
  char[] toString() { return name.str; }

  // Ask the variable if we can write to it.
  bool isLValue()
    {
      // Specials are read only
      if(isSpecial)
        return false;

      // Properties may or may not be changable
      if(isProperty)
        return look.isPropLValue;

      // Normal variables are always lvalues.
      return true;
    }

  bool isStatic()
    {
      // Properties can be static
      if(isProperty)
        return look.isPropStatic;

      // Type names are always static.
      if(isType)
        return true;

      // Currently no other static variables
      return false;
    }

  bool isCTime() { return isType; }

  void parse(ref TokenArray toks)
    {
      name = next(toks);
      loc = name.loc;
    }

  void resolve(Scope sc)
    out
    {
      // Some sanity checks on the result
      if(look.isVar)
        {
          assert(look.var.sc !is null);
          assert(!isProperty);
        }
      assert(type !is null);
      assert(vtype != VType.None);
    }
    body
    {
      if(isMember) // Are we called as a member?
	{
          // Look up the name in the scope belonging to the owner
          assert(leftScope !is null);
          look = leftScope.lookup(name);

          type = look.type;

          // Check first if this is a variable
          if(look.isVar)
            {
              // We are a class member variable.
              vtype = VType.FarOtherVar;
              assert(look.sc.isClass);

              return;
            }

          // Check for properties
          if(look.isProperty)
            {
              // TODO: Need to account for ownerType here somehow -
              // rewrite the property system
              vtype = VType.Property;
              type = look.getPropType(ownerType);
              return;
            }

          // Check types too
          if(look.isType)
            {
              vtype = VType.Type;
              type = look.type.getMeta();
              return;
            }

          // No match
          fail(name.str ~ " is not a variable member of " ~ ownerType.toString,
               loc);
        }

      // Not a member

      // Look for reserved names first.
      if(name.str == "__STACK__")
        {
          vtype = VType.Special;
          type = BasicType.getInt;
          return;
        }

      if(name.type == TT.Const || name.type == TT.Clone)
        fail("Cannot use " ~ name.str ~ " as a variable", name.loc);

      // Not a member or a special name. Look ourselves up in the
      // local scope, and include imported scopes.
      look = sc.lookupImport(name);

      if(look.isImport)
        {
          // We're imported from another scope. This means we're
          // essentially a member variable. Let DotOperator handle
          // this.

          dotImport = new DotOperator(look.imphold, this, loc);
          dotImport.resolve(sc);
          return;
        }

      // These are special cases that work both as properties
      // (object.state) and as non-member variables (state=...) inside
      // class functions / state code. Since we already handle them
      // nicely as properties, treat them as properties even if
      // they're not members.
      if(name.type == TT.Singleton || name.type == TT.State)
        {
          if(!sc.isInClass)
            fail(name.str ~ " can only be used in classes", name.loc);

          assert(look.isProperty, name.str ~ " expression not implemented yet");

          vtype = VType.Property;
          type = look.getPropType(ownerType);
          return;
        }

      type = look.type;

      if(look.isVar)
        {
          assert(look.sc !is null);
          assert(look.sc is look.var.sc);

          // Class variable?
          if(look.sc.isClass)
            {
              // Check if it's in THIS class, which is a common
              // case. If so, we can use a simplified instruction that
              // doesn't have to look up the class.
              if(look.sc.getClass is sc.getClass)
                vtype = VType.ThisVar;
              else
                {
                  // It's another class. For non-members this can only
                  // mean a parent class.
                  vtype = VType.ParentVar;
                }
            }
          else
            vtype = VType.LocalVar;

          return;
        }

      // We are not a variable. Last chance is a type name / class.
      if(!look.isType && !look.isClass)
        {
          // Still no match. Might be an unloaded class however,
          // lookup() doesn't load classes. Try loading it.
          if(global.findParsed(name.str) is null)
            // No match at all.
            fail("Undefined identifier "~name.str, name.loc);

          // We found a class! Check that we can look it up now
          look = sc.lookup(name);
          assert(look.isClass);
        }

      vtype = VType.Type;

      // Class name?
      if(look.isClass)
        {
          assert(look.mc !is null);
          look.mc.requireScope();

          type = look.mc.classType;

          // Singletons are treated differently - the class name can
          // be used to access the singleton object
          if(look.mc.isSingleton)
            {
              type = look.mc.objType;
              singCls = look.mc.getIndex();
            }
        }
      else
        {
          assert(look.type !is null);
          type = look.type.getMeta();
        }
    }

  int[] evalCTime()
    {
      assert(isCTime);
      assert(isType);
      assert(type.getSize == 0);
      return null;
    }

  void evalAsm()
    {
      // Hairy. But does the trick for now.
      if(dotImport !is null && recurse)
        {
          recurse = false;
          dotImport.evalAsm();
          return;
        }

      if(isType) return;

      setLine();

      // Special name
      if(isSpecial)
	{
	  if(name.str == "__STACK__")
	    tasm.getStack();
          else assert(0, "Unknown special name " ~ name.str);
	  return;
	}

      // Property
      if(isProperty)
        {
          look.getPropValue();
          return;
        }

      // Class singleton name
      if(singCls != -1)
        {
          assert(type.isObject);

          // Convert the class index into a object index at runtime
          tasm.pushSingleton(singCls);
          return;
        }

      // Normal variable

      int s = type.getSize;
      auto var = look.var;

      if(vtype == VType.LocalVar)
        // This is a variable local to this function. The number gives
        // the stack position.
        tasm.pushLocal(var.number, s);

      else if(vtype == VType.ThisVar)
        // The var.number gives the offset into the data segment in
        // this class
        tasm.pushClass(var.number, s);

      else if(vtype == VType.ParentVar)
        // Variable in a parent but this object
        tasm.pushParentVar(var.number, var.sc.getClass().getTreeIndex(), s);

      else if(vtype == VType.FarOtherVar)
        // Push the value from a "FAR pointer". The class index should
        // already have been pushed on the stack by DotOperator, we
        // only push the index.
        tasm.pushFarClass(var.number, var.sc.getClass().getTreeIndex(), s);

      else assert(0);
    }

  // Push the address of the variable rather than its value
  void evalDest()
    {
      if(dotImport !is null && recurse)
        {
          recurse = false;
          dotImport.evalDest();
          return;
        }

      assert(!isType, "types can never be written to");
      assert(isLValue());
      assert(!isProperty);

      setLine();

      // No size information is needed for addresses.
      auto var = look.var;

      if(vtype == VType.LocalVar)
        tasm.pushLocalAddr(var.number);
      else if(vtype == VType.ThisVar)
        tasm.pushClassAddr(var.number);
      else if(vtype == VType.ParentVar)
        tasm.pushParentVarAddr(var.number, var.sc.getClass().getTreeIndex());
      else if(vtype == VType.FarOtherVar)
        tasm.pushFarClassAddr(var.number, var.sc.getClass().getTreeIndex());

      else assert(0);
    }

  void store()
    {
      if(dotImport !is null && recurse)
        {
          recurse = false;
          dotImport.store();
          return;
        }

      assert(isLValue);

      if(isProperty)
        look.setPropValue();
      else
        {
          assert(look.isVar);
          auto var = look.var;

          // Get the destination and move the data
          evalDest();
          tasm.mov(type.getSize());

          assert(var.sc !is null);
          if(var.isRef)
            // TODO: This assumes all ref variables are foreach values,
            // which will probably not be true in the future.
            tasm.iterateUpdate(var.sc.getLoopStack());
        }
    }

  void incDec(TT op, bool post)
    {
      if(dotImport !is null && recurse)
        {
          recurse = false;
          dotImport.incDec(op, post);
          return;
        }

      if(!isProperty)
        {
          assert(look.isVar);
          auto var = look.var;

          super.incDec(op, post);

          assert(var.sc !is null);
          if(var.isRef)
            tasm.iterateUpdate(var.sc.getLoopStack());
        }
      else fail("Cannot use ++ and -- on properties yet", loc);
    }
}

// A variable declaration that works as a statement. Supports multiple
// variable declarations, ie. int i, j; but they must be the same
// type, so int i, j[]; is not allowed.
class VarDeclStatement : Statement
{
  VarDeclaration[] vars;

  static bool canParse(TokenArray toks)
    {
      if(Type.canParseRem(toks) &&
	 isNext(toks, TT.Identifier))
	return true;
      return false;
    }

  void parse(ref TokenArray toks)
    {
      VarDeclaration varDec;
      varDec = new VarDeclaration;
      varDec.parse(toks);
      vars ~= varDec;
      loc = varDec.var.name.loc;

      int arr = varDec.arrays();

      // Are there more?
      while(isNext(toks, TT.Comma))
	{
	  // Read a variable, but with the same type as the last
	  varDec = new VarDeclaration(varDec.var.type);
	  varDec.parse(toks);
	  if(varDec.arrays() != arr)
	    fail("Multiple declarations must have same type",
                 varDec.var.name.loc);
	  vars ~= varDec;
	}

      if(!isNext(toks, TT.Semicolon))
	fail("Declaration statement expected ;", toks);
    }

  char[] toString()
    {
      char[] res = "Variable declaration: ";
      foreach(vd; vars) res ~= vd.toString ~" ";
      return res;
    }

  void resolve(Scope sc)
    {
      if(sc.isStateCode())
	fail("Variable declarations not allowed in state code", loc);

      // Add variables to the scope.
      foreach(vd; vars)
	vd.resolve(sc);
    }

  // Validate types
  void validate()
    {
      assert(vars.length >= 1);
      vars[0].var.type.validate(loc);
    }

  // Insert local variable(s) on the stack.
  void compile()
    {
      // Compile the variable declarations, they will push the right
      // values to the stack.
      foreach(vd; vars)
	vd.compile();
    }
}
