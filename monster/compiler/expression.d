/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (expression.d) is part of the Monster script language
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

module monster.compiler.expression;

import monster.compiler.tokenizer;
import monster.compiler.scopes;
import monster.compiler.operators;
import monster.compiler.types;
import monster.compiler.assembler;
import monster.compiler.block;
import monster.compiler.variables;
import monster.compiler.functions;

import monster.vm.error;
import monster.vm.mclass;
import monster.vm.arrays;
import monster.util.list;

import std.string;
import std.stdio;
import std.utf : decode, toUTF32;

alias Expression[] ExprArray;

// An expression is basically anything that can return a value,
// including a 'void'.
abstract class Expression : Block
{
  // Identify a part of an expression. The sub expressions include all
  // expressions except binary operators, and it is used in identify()
  // to parse the parts between such operators. All lvalues can be
  // parsed with this function.
  static Expression identifySub(ref TokenArray toks, bool isMember = false)
    /*
    out(res)
    {
      writefln("identifySub returned ", res);
    }
    body
    //*/
    {
      Expression b;
      Floc ln;

      // These are allowed for members (eg. a.hello().you())
      if(FunctionCallExpr.canParse(toks)) b = new FunctionCallExpr;
      else if(VariableExpr.canParse(toks)) b = new VariableExpr;
      else if(isMember) fail(toks[0].str ~ " can not be a member", toks[0].loc);

      // The rest are not allowed for members (eg. con.(a+b) is not
      // allowed)
      else if(NewExpression.canParse(toks)) b = new NewExpression;
      else if(LiteralExpr.canParse(toks)) b = new LiteralExpr;
      else if(ArrayLiteralExpr.canParse(toks)) b = new ArrayLiteralExpr;
      // Sub-expression (expr)
      else if(isNext(toks, TT.LeftParen))
	{
	  b = readParens(toks);
	  goto noParse;
	}
      // Unary operators, op expr
      else if(UnaryOperator.canParse(toks)) b = new UnaryOperator;

      else if(isNext(toks, TT.Semicolon, ln))
	fail("Use {} for empty statements, not ;", ln);

      else fail("Cannot interpret expression " ~ toks[0].str, toks[0].loc);

      b.parse(toks);

    noParse:

      // Check for . expressions and resolve the members as such. If
      // the sub-expression b is followed by a . then the following
      // tokens must be parsed as a new sub-expression that is a
      // member of b. The problem is to call identifySub recursively
      // so that expressions are ordered correctly,
      // ie. first.second.third becomes (((first).second).third). The
      // solution (relying on isMember) is a bit quirky but it
      // works. Dots were originally handled as normal binary
      // operators, but that gave wrong syntax in some corner
      // cases. There is also the problem that operators such as ++
      // and [] should apply to the subexpression as a whole, not the
      // member, so we put those in here.
      if(!isMember)
	{
          while(1)
            {
              if(isNext(toks, TT.Dot, ln))
                b = new DotOperator(b, identifySub(toks, true), ln);

              // After any sub-expression there might be a [] or [expr] to
              // indicate array operation. We check for that here, and
              // loop since the result is a subexpression which might
              // itself contain more []s after it. We allow expressions
              // like 20[5] syntactically, since these will be weeded out
              // at the semantic level anyway.
              else if(isNext(toks,TT.LeftSquare))
                {
                  // Empty []?
                  if(isNext(toks, TT.RightSquare, ln))
                    b = new ArrayOperator(b, ln);
                  else // [expr] or [expr..expr]
                    {
                      Expression exp = identify(toks);

                      if(isNext(toks, TT.DDot))
                        // b[exp .. exp2]
                        b = new ArrayOperator(b, ln,  exp, identify(toks));
                      else
                        // b[exp]
                        b = new ArrayOperator(b, ln, exp);

                      if(!isNext(toks, TT.RightSquare))
                        fail("Array expected closing ]", toks);
                    }
                }
              else break;
            }
          // Finally, check for a single ++ or -- following an expression.
          if(isNext(toks, TT.PlusPlus)) b = new UnaryOperator(b, true);
          else if(isNext(toks, TT.MinusMinus)) b = new UnaryOperator(b, false);
	}

      return b;
    }

 private:
  // This reads the contents of a parenthesis pair (...). It really
  // just calls identify() again and removes the paren at the
  // end. However, this ensures that operator precedence takes place
  // locally inside the paren pair, and that the result is seen as one
  // unit to the outside expression. And this is of course what
  // parentheses are for.
  static Expression readParens(ref TokenArray toks)
    {
      // We assume the opening parenthesis has been removed
      // already. Let's get to parsin'!
      Expression res = identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("Expected ) after expression", toks);

      return res;
    }

  // Used in parsing expressions
  struct ExpOp
  {
    Expression exp;
    TT nextOp; // Operator to the right of the expression
    Floc loc;
  }

  // Operators handled below. Don't really need a special function for
  // this...
  static bool getNextOp(ref TokenArray toks, ref Token t)
    {
      if(toks.length == 0) return false;
      TT tt = toks[0].type;
      if(/*tt == TT.Dot || */tt == TT.Equals || tt == TT.Plus ||
	 tt == TT.Minus || tt == TT.Mult || tt == TT.Div ||
	 tt == TT.Rem || tt == TT.IDiv ||

	 tt == TT.PlusEq || tt == TT.MinusEq || tt == TT.MultEq ||
	 tt == TT.DivEq || tt == TT.RemEq || tt == TT.IDivEq ||
         tt == TT.CatEq ||

	 tt == TT.Cat || 

	 tt == TT.IsEqual || tt == TT.Less || tt == TT.More ||
	 tt == TT.LessEq || tt == TT.MoreEq || tt == TT.NotEqual ||
	 tt == TT.And || tt == TT.Or ||

         tt == TT.IsCaseEqual || tt == TT.NotCaseEqual)
	{
	  t = next(toks);
	  return true;
	}
      return false;
    }

 public:

  // This represents the type of this expression. It must be created
  // and set up (resolved) in the child classes.
  Type type;

  // Parse an entire expression
  static Expression identify(ref TokenArray toks)
    /*
    out(res)
    {
      writefln("identify returned ", res);
    }
    body
    //*/
    {
      // Create a linked list to store the expressions and operators.
      LinkedList!(ExpOp) exprList;

      ExpOp eo;
      Token tt;

      do
	{
	  // Parse the next sub-expression
	  eo.exp = identifySub(toks);

	  // Check if this expression has an operator behind it. If
	  // not, it is the last expression.
	  if(getNextOp(toks, tt))
	    {
	      eo.nextOp = tt.type;
	      eo.loc = tt.loc;
	    }
	  else
	    eo.nextOp = TT.EOF;

	  // Insert the (expr+op) pair.
	  exprList.insert(eo);
	}
      while(eo.nextOp != TT.EOF);

      // Replaces a pair (expr1+op1,expr2,op2) with (newExpr,op2),
      // where newExpr is created from (expr1 op1 expr2).
      // eg. converts a+b*c to a + (b*c). It takes the right
      // expression as parameter, and removes the left, so that it can
      // be called while iterating the list.
      void replace(exprList.Iterator right, bool assign = false, bool boolop=false)
	{
	  auto left = right.getPrev;
	  assert(left != null);

	  // Create the compound expression. Replace the right node,
	  // since it already has the correct operator.
	  if(assign)
	    right.value.exp = new AssignOperator(left.value.exp,
						 right.value.exp,
						 left.value.nextOp,
						 left.value.loc);
	  else if(boolop)
	    right.value.exp = new BooleanOperator(left.value.exp,
						 right.value.exp,
						 left.value.nextOp,
						 left.value.loc);
	  else
	    right.value.exp = new BinaryOperator(left.value.exp,
						 right.value.exp,
						 left.value.nextOp,
						 left.value.loc);

	  // Kill the left node.
	  exprList.remove(left);
	}

      static bool has(TT list[], TT type)
	{
	  foreach(tt; list) if(tt == type) return true;
	  return false;
	}

      // Find all expression pairs bound together with one of the
      // given operator types
      void find(TT types[] ...)
	{
	  auto it = exprList.getHead();
	  while(it !is null)
	    {
	      // Is it the right operator?
	      if(types.has(it.value.nextOp))
		{
		  // Replace it and continue to the next element
		  it = it.getNext();
		  replace(it);
		}
	      else
		it = it.getNext();
	    }
	}

      // Boolean operators
      void findBool(TT types[] ...)
	{
	  auto it = exprList.getHead();
	  while(it !is null)
	    {
	      // Is it the right operator?
	      if(types.has(it.value.nextOp))
		{
		  // Replace it and continue to the next element
		  it = it.getNext();
		  replace(it, false, true);
		}
	      else
		it = it.getNext();
	    }
	}

      // As find(), but searches from the right, and inserts
      // assignment operators.
      void findAssign(TT types[] ...)
	{
	  auto it = exprList.getTail();
	  while(it !is null)
	    {
	      // Is it the right operator?
	      if(types.has(it.value.nextOp))
		{
		  // Find the next element to the left
		  auto nxt = it.getPrev();
		  replace(it.getNext(), true);
		  it=nxt;
		}
	      else
		it = it.getPrev();
	    }
	}

      // Now sort through the operators according to precedence. This
      // is the precedence I use, it should be ok. (Check it against
      // something else)

      // / %
      // *
      // + -
      // ~
      // == != < > <= >= =i= =I= !=i= !=I=
      // || &&
      // = += -= *= /= %= ~=

      // Dot operators are now handled in identifySub
      //find(TT.Dot);

      find(TT.Div, TT.Rem, TT.IDiv);

      find(TT.Mult);

      find(TT.Plus, TT.Minus);

      find(TT.Cat);

      findBool(TT.IsEqual, TT.NotEqual, TT.Less, TT.More, TT.LessEq, TT.MoreEq,
               TT.IsCaseEqual, TT.NotCaseEqual);

      findBool(TT.Or, TT.And);

      // Find assignment operators. These use a different Expression
      // class and are searched in reverse order.
      findAssign(TT.Equals, TT.PlusEq, TT.MinusEq, TT.MultEq, TT.DivEq,
		  TT.RemEq, TT.IDivEq, TT.CatEq);

      assert(exprList.length == 1, "Cannot reduce expression list to one element");
      return exprList.getHead().value.exp;
    }

  // Get a sensible line number for this expression. Used in error
  // messages.
  Floc getLoc() { return loc; }

  // Special version of resolve that is called for members,
  // ie. objname.member;. Most expressions cannot be members,
  // but those that can must override this.
  void resolveMember(Scope sc, Type ownerType)
    { fail(toString() ~ " cannot be a member of " ~ ownerType.toString, loc); }

  // Used for error messages
  char[] typeString() { return type.toString(); }

  // Can this expression be assigned to? (most types can not)
  bool isLValue() { return false; }

  // If true, assignments to this expression (for lvalues) are handled
  // through writeProperty rather than with evalDest.
  bool isProperty() { return false; }

  // Static expressions. These can be evaluated as members without
  // needing the owner pushed onto the stack.
  bool isStatic() { return false; }

  // Evaluate this using run-time instructions. This is only used when
  // evalCTime can not be used (ie. when isCTime is false.)
  void evalAsm() { fail(format("expression ", this, " not implemented")); }

  // This is the equivalent of 'compile' for statements. Create
  // compiled code that evaluates the expression. The result should be
  // the value of the expression pushed onto the stack. Uses compile
  // time information when available.
  final void eval()
    {
      if(isCTime())
        {
          int[] data = evalCTime();
          assert(data.length == type.getSize);
          tasm.pushArray(data);
        }
      else
        evalAsm();
    }

  // Evaluate and pop the value afterwards. Might be optimized later.
  final void evalPop()
    {
      eval();
      setLine();
      if(!type.isVoid)
        tasm.pop(type.getSize());
    }

  // Evaluate this expression as a destination (ie. push a pointer
  // instead of a value). Only valid for LValues. TODO: This
  // completely replaces the need for a separate isLValue member, but
  // I think I will keep it. It belongs in the semantic level, while
  // this is in the code generation level.
  void evalDest() { assert(0, "evalDest() called for non-lvalue " ~ this.toString); }

  // Called on all lvalue expressions after they have been
  // modified, and must be stack-neutral.
  void postWrite() { assert(isLValue()); }

  // Assign to this property. Only called if isProperty returns true.
  void writeProperty() { assert(0); }

  // Can this expression be evaluated at compile time?
  bool isCTime() { return false; }

  // Return the compile-time value of this expression
  int[] evalCTime() { assert(0); return null; }
}

// new-expressions, ie. (new Sometype[]).
class NewExpression : Expression
{
  // The array of expressions inside brackets. Examples:
  // new int[5][10]  -  contains the expressions 5 and 10
  // new int[][][2] - contains the expressions null, null and 2
  ExprArray exArr;

  // Base type of our array. Examples:
  // new int[10]     - base is int
  // new int[10][10] - base is int
  // new int[][10]   - base is int[]
  Type baseType;

  CIndex clsInd;

  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.New); }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.New, loc))
	assert(0, "Internal error in NewExpression");

      type = Type.identify(toks, true, exArr);
    }

  char[] toString()
    {
      return "(new " ~ typeString ~ ")";
    }

  void resolve(Scope sc)
    {
      type.resolve(sc);

      if(type.isObject)
        {
          // We need to find the index associated with this class, and
          // pass it to the assembler.
          clsInd = (cast(ObjectType)type).getClass().getIndex();
        }
      else if(type.isArray)
        {
          assert(exArr.length == type.arrays);

          // Used for array size specifiers
          Type intType = BasicType.getInt;

          // The remaining expressions must fill toward the right. For
          // example, [1][2][3] is allowed, as is [][][1][2], but not
          // [1][][2].
          bool found=false;
          int firstIndex = -1; // Index of the first non-empty expression
          foreach(int i, ref Expression e; exArr)
            if(e !is null)
              {
                if(!found)
                  {
                    // The first non-empty expression! Store the
                    // index.
                    found = true;
                    firstIndex = i;
                  }

                // Resolve the expressions while we're at it.
                e.resolve(sc);

                // Check the type
                try intType.typeCast(e);
                catch(TypeException)
                  fail("Cannot convert array index " ~ e.toString ~ " to int", loc);
              }
            else
              {
                if(found) // Cannot have an empty expression after a
                          // non-empty one.
                  fail("Invalid array specifier in 'new' expression", loc);
              }

          if(firstIndex == -1)
            fail("Right-most bracket in 'new' expression must contain an expression",
                 loc);

          // This is already true from the above check, we're
          // defensive here.
          assert(exArr[$-1] !is null);

          // Only store non-empty expressions, since those are the
          // only ones we will act on.
          exArr = exArr[firstIndex..$];

          // Find the base type of our allocation. This is used to
          // find the initialization value and size.
          baseType = type;
          for(int i=exArr.length; i>0; i--)
            baseType = baseType.getBase();
        }
      else
	fail("Cannot use 'new' with type " ~ type.toString, loc);
    }

  void evalAsm()
    {
      setLine();

      if(type.isObject)
        {
          // Create a new object. This is done through a special byte code
          // instruction.
          tasm.newObj(clsInd);
        }
      else if(type.isArray)
        {
          int initVal[] = baseType.defaultInit();

          int rank = exArr.length; // Array nesting level

          // Check that the numbers add up
          assert(type.arrays == baseType.arrays + rank);

          // Push the lengths on the stack
          foreach(ex; exArr)
            ex.eval();

          setLine();

          // Create an array with the given initialization and
          // dimensions. The init value can be any length (number of
          // ints), and elements are assumed to be the same length.
          tasm.newArray(initVal, rank);
        }
      else assert(0, "not implemented yet");
    }
}

// Array literals [expr,expr,...]. Does not cover string literals,
// those are covered by LiteralExpr below.
class ArrayLiteralExpr : Expression
{
  ExprArray params;

  MonsterClass cls; // Needed to insert static arrays
  AIndex arrind; // Do not remove! Used for semi-permanent slices.

  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.LeftSquare); }

  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.LeftSquare, loc))
	assert(0, "Internal error in ArrayLiteralExpr");

      Floc loc2;
      if(isNext(toks, TT.RightSquare, loc2))
	fail("Array literal cannot be empty", loc2);

      // Read the first expression
      params ~= Expression.identify(toks);

      // Check for more expressions
      while(isNext(toks, TT.Comma))
	params ~= Expression.identify(toks);

      if(!isNext(toks, TT.RightSquare))
	fail("Array literal expected closing ]", toks);
    }

  char[] toString()
    {
      char[] res = " [";
      foreach(expr; params[0..params.length-1])
	res ~= expr.toString ~ ", ";
      return res ~ params[params.length-1].toString ~ "] ";
    }

  bool isCTime()
    {
      foreach(p; params)
        if(!p.isCTime) return false;
      return true;
    }

  void resolve(Scope sc)
    {
      foreach(expr; params)
	expr.resolve(sc);

      assert(params.length != 0);

      // Set the type
      Type base = params[0].type;
      type = new ArrayType(base);

      foreach(ref par; params)
	{
	  // Check that all elements are of a usable type, and convert
	  // if necessary.
          try base.typeCast(par);
          catch(TypeException)
            fail(format("Cannot convert %s of type %s to type %s", par,
                        par.typeString(), params[0].typeString()), getLoc);
	}

      cls = sc.getClass();

      if(isCTime)
        cls.reserveStatic(params.length * base.getSize);
    }

  int[] evalCTime()
    {
      assert(isCTime());

      // Element size and number
      int elem = type.getBase().getSize();
      int num = params.length;

      // Create a temporary array containing the data
      const int LEN = 32;
      int buf[LEN];
      int data[];

      if(num*elem <= LEN)
        data = buf[0..num*elem];
      else
        data = new int[num*elem];

      // Set up the data
      foreach(i, par; params)
        data[i*elem..(i+1)*elem] = par.evalCTime();

      // Insert the array into the static data, and get the array
      // reference
      arrind = cls.insertStatic(data, elem);

      // Delete the array, if necessary
      if(data.length > LEN)
        delete data;

      // Create an array from the index and return it as the compile
      // time value
      return (cast(int*)&arrind)[0..1];
    }

  void evalAsm()
    {
      assert(!isCTime());

      int s = params[0].type.getSize;
      assert(s >= 1);

      // Push all the elements on the stack
      foreach(par; params)
        {
          assert(par.type.getSize == s);
          par.eval();
        }

      setLine();

      // And simply pop them back into an array
      tasm.popToArray(params.length, s);
    }
}

// Expression representing a literal or other special single-token
// values. Supported tokens are StringLiteral, NumberLiteral,
// CharLiteral, True, False, Null, Dollar and This. Array literals are
// handled by ArrayLiteralExpr.
class LiteralExpr : Expression
{
  Token value;

  // Values (used depending on type)
  int ival;
  dchar dval; // Characters are handled internally as dchars
  float fval;

  // TODO/FIXME: When evalutationg the array length symbol $, we
  // evaluate the array expression again, and the find its
  // length. This is a TEMPORARY solution - if the array expression is
  // a complicated expression or if it has side effects, then
  // evaluating it more than once is obviously not a good idea. Example:
  // myfunc()[$-4..$]; // myfunc() is called three times!

  // A better solution is to store the array index on the stack for
  // the entire duration of the array expression and remember the
  // position, just like we do for foreach. Since the index has
  // already been pushed, this is pretty trivial to do through the
  // scope system.
  Expression arrayExp;

  // TODO: Does not support double, long or unsigned types yet. A much
  // more complete implementation will come later.

  // String, with decoded escape characters etc and converted to
  // utf32. We need the full dchar string here, since we might have to
  // handle compile-time expressions like "hello"[2..4], which should
  // return the right characters.
  dchar[] strVal;
  AIndex arrind; // Placed here because we return a permanent slice of it

  // Used for inserting string data into the data segment.
  MonsterClass cls;

  static bool canParse(TokenArray toks)
    {
      return
	isNext(toks, TT.StringLiteral) ||
	isNext(toks, TT.NumberLiteral) ||
	isNext(toks, TT.CharLiteral) ||
	isNext(toks, TT.True) ||
	isNext(toks, TT.False) ||
	isNext(toks, TT.Null) ||
        isNext(toks, TT.Dollar) ||
	isNext(toks, TT.This);
    }

  void parse(ref TokenArray toks)
    {
      value = next(toks);
      loc = value.loc;
    }

  char[] toString()
    {
      return value.str;
    }

  bool isRes = false;

  void resolve(Scope sc)
    {
      isRes = true;

      // Find the class and store it for later
      cls = sc.getClass();

      // The 'this' name refers to the current object, and is the same
      // type as the current class.
      if(value.type == TT.This)
	{
	  // Get the type from the current class
          type = sc.getClass().objType;
	  return;
	}

      bool hasPercent()
        {
          int i = value.str.find('%');
          if(i == -1) return false;

          // Make sure it is at the end
          if(i != value.str.length-1)
            fail("Number literals can only have a percentage sign (%) at the end. Perhaps you meant the reminder operator '%%' ?", value.loc);

          return true;
        }

      // Numeric literal.
      if(value.type == TT.NumberLiteral)
	{
	  // Parse number strings. Simple hack for now, assume it's an
	  // int unless it contains a period, then it's a float. TODO:
	  // Improve this later, see how it is done elsewhere.
	  if(value.str.find('.') != -1 || hasPercent())
	    {
              type = BasicType.getFloat;
	      fval = atof(value.str);

              if(hasPercent())
                fval /= 100;

	      return;
	    }

          type = BasicType.getInt;
	  ival = atoi(value.str);
	  return;
	}

      // The $ token. Only allowed in array indices.
      if(value.type == TT.Dollar)
        {
          if(!sc.isArray)
            fail("Array length $ not allowed here", loc);

          type = BasicType.getInt;
          arrayExp = sc.getArray();
          return;
        }

      // The token 'null'
      if(value.type == TT.Null)
        {
          // 'null' has a special type that is converted when needed
          type = new NullType;
          return;
        }

      // Bool literal
      if(value.type == TT.True || value.type == TT.False)
	{
          type = BasicType.getBool;
	  if(value.type == TT.True)
	    ival = 1;
	  else
	    ival = 0;
	  return;
	}

      // Single character
      if(value.type == TT.CharLiteral)
	{
          type = BasicType.getChar;

          // Decode the unicode character. TODO: Error checking?
          // Unicode sanity checks should be handled in the tokenizer.
          size_t idx = 1;
	  dval = decode(value.str, idx);
	  return;
	}

      // Strings
      if(value.type == TT.StringLiteral)
        {
          type = new ArrayType(BasicType.getChar);

          // Check that we do indeed have '"'s at the ends of the
          // string. Special cases which we allow later (like wysiwig
          // strings, @"c:\") will have their special characters
          // removed in the tokenizer.
          assert(value.str.length >=2 && value.str[0] == '"' && value.str[$-1] == '"',
                 "Encountered invalid string literal token: " ~ value.str);

          strVal = toUTF32(value.str[1..$-1]);
          cls.reserveStatic(strVal.length);

          return;
        }

      fail("Unhandled literal type " ~ value.str, loc);
    }

  // We currently support a few kinds of constants
  bool isCTime()
    {
      if(value.type == TT.Dollar) return false;

      return
        type.isInt() || type.isBool() || type.isFloat || type.isChar ||
        value.type == TT.Null || type.isString;
    }

  int[] evalCTime()
    {
      // Return a slice of the value
      if(type.isInt || type.isBool) return (&ival)[0..1];
      if(type.isChar) return (cast(int*)&dval)[0..1];
      if(type.isFloat) return (cast(int*)&fval)[0..1];

      if(type.isString)
        {
          // Insert the array into the static data, and get the array
          // reference
          arrind = cls.insertStatic(cast(int[])strVal, 1);
          // Create an array from it
          return (cast(int*)&arrind)[0..1];
        }

      // Let the type cast from NullType create the data
      if(value.type == TT.Null) return null;

      assert(0, "Compile time evaluation of " ~ toString ~ " not implemented yet");
    }

  void evalAsm()
    {
      assert(!isCTime());
      assert(type !is null, "not resolved");
      setLine();

      if(value.type == TT.Dollar)
        {
          // Get the array. TODO/FIXME: This is a very bad solution,
          // the entire array expression is recomputed whenever we use
          // the $ symbol. If the expression has side effects (like a
          // function call), this can give unexpected results. This is
          // a known bug that will be fixed later. The simplest
          // solution is to let ArrayExpression create a new scope,
          // which stores the stack position of the array index.
          arrayExp.eval();
          // Convert it to the length
          setLine();
          tasm.getArrayLength();
        }
      else if(value.type == TT.This) tasm.pushThis();
      else fail("Literal type '" ~ value.str ~ "' not supported yet", loc);
    }
}

// Expressions that can be members of other types. Can be variables,
// types or functions.
abstract class MemberExpression : Expression
{
  Scope leftScope;
  bool isMember = false;
  Type ownerType;

  void resolveMember(Scope sc, Type ownerT)
    {
      assert(ownerT !is null);
      leftScope = ownerT.getMemberScope();
      ownerType = ownerT;
      isMember = true;

      resolve(sc);
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
  Variable *var;
  Property prop;

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
  int classIndex = -1;  // Index of the class that owns this variable.

  static bool canParse(TokenArray toks)
    {
      return
        isNext(toks, TT.Identifier) ||
        isNext(toks, TT.Singleton) ||
        isNext(toks, TT.State) ||
        isNext(toks, TT.Const);
    }

  void parse(ref TokenArray toks)
    {
      name = next(toks);
      loc = name.loc;
    }

  // Does this variable name refer to a type name rather than an
  // actual variable?
  bool isType()
    {
      return type.isMeta();
    }

  bool isProperty()
  out(res)
  {
    if(res)
      {
        assert(prop.name != "");
        assert(var is null);
        assert(!isSpecial);
      }
    else
      {
        assert(prop.name == "");
      }
  }
  body
  {
    return vtype == VType.Property;
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
        return prop.isLValue;

      // Normal variables are always lvalues.
      return true;
    }

  bool isStatic()
    {
      // Properties can be static
      if(isProperty)
        return prop.isStatic;

      // Type names are always static (won't be true for type
      // variables later, though.)
      if(isType)
        return true;

      return false;
    }

  // TODO: isCTime - should be usable for static properties and members

  void writeProperty()
    {
      assert(isProperty);
      prop.setValue();
    }

  void resolve(Scope sc)
    out
    {
      // Some sanity checks on the result
      if(isProperty) assert(var is null);
      if(var !is null)
        {
          assert(var.sc !is null);
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

          // Check first if this is a variable
          var = leftScope.findVar(name.str);
          if(var !is null)
            {
              // We are a member variable
              type = var.type;

              // The object pointer is pushed on the stack. We must
              // also provide the class index, so the variable is
              // changed in the correct class (it could be a parent
              // class of the given object.)
              vtype = VType.FarOtherVar;
              assert(var.sc.isClass);
              classIndex = var.sc.getClass().getIndex();

              return;
            }

          // Check for properties last
          if(leftScope.findProperty(name, ownerType, prop))
            {
              // We are a property
              vtype = VType.Property;
              type = prop.getType;
              return;
            }

          // No match
          fail(name.str ~ " is not a member of " ~ ownerType.toString,
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

      if(name.type == TT.Const)
        fail("Cannot use const as a variable", name.loc);

      // These are special cases that work both as properties
      // (object.state) and as non-member variables (state=...) inside
      // class functions / state code. Since we already handle them
      // nicely as properties, treat them as properties.
      if(name.type == TT.Singleton || name.type == TT.State)
        {
          if(!sc.isInClass)
            fail(name.str ~ " can only be used in classes", name.loc);

          if(!sc.findProperty(name, sc.getClass().objType, prop))
            assert(0, "should have found property " ~ name.str ~
                   " in scope " ~ sc.toString);

          vtype = VType.Property;
          type = prop.getType;
          return;
        }

      // Not a member, property or a special name. Look ourselves up
      // in the local variable scope.
      var = sc.findVar(name.str);

      if(var !is null)
        {
          type = var.type;

          assert(var.sc !is null);

          // Class variable?
          if(var.sc.isClass)
            {
              // Check if it's in THIS class, which is a common
              // case. If so, we can use a simplified instruction that
              // doesn't have to look up the class.
              if(var.sc.getClass is sc.getClass)
                vtype = VType.ThisVar;
              else
                {
                  // It's another class. For non-members this can only
                  // mean a parent class.
                  vtype = VType.ParentVar;
                  classIndex = var.sc.getClass().getIndex();
                }
            }
          else
            vtype = VType.LocalVar;

          return;
        }

      // We are not a variable. Maybe we're a basic type?
      if(BasicType.isBasic(name.str))
        {
          // Yes!
          type = MetaType.get(name.str);
          vtype = VType.Type;
          return;
        }

      // No match at all
      fail("Undefined identifier "~name.str, name.loc);
    }

  void evalAsm()
    {
      // If we are a type, just do nothing. If the type is used for
      // anything, it will be typecast and all the interesting stuff
      // will be handled by the type system.
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
          prop.getValue();
          return;
        }

      // Normal variable

      int s = type.getSize;

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
        tasm.pushParentVar(var.number, classIndex, s);

      else if(vtype == VType.FarOtherVar)
        // Push the value from a "FAR pointer". The class index should
        // already have been pushed on the stack by DotOperator, we
        // only push the index.
        tasm.pushFarClass(var.number, classIndex, s);

      else assert(0);
    }

  // Push the address of the variable rather than its value
  void evalDest()
    {
      assert(!isType, "types can never be written to");
      assert(isLValue());
      assert(!isProperty);

      setLine();

      // No size information is needed for addresses.

      if(vtype == VType.LocalVar)
        tasm.pushLocalAddr(var.number);
      else if(vtype == VType.ThisVar)
        tasm.pushClassAddr(var.number);
      else if(vtype == VType.ParentVar)
        tasm.pushParentVarAddr(var.number, classIndex);
      else if(vtype == VType.FarOtherVar)
        tasm.pushFarClassAddr(var.number, classIndex);

      else assert(0);
    }

  void postWrite()
    {
      assert(!isProperty);
      assert(isLValue());
      assert(var.sc !is null);
      if(var.isRef)
        // TODO: This assumes all ref variables are foreach values,
        // which will probably not be true in the future.
        tasm.iterateUpdate(var.sc.getLoopStack());
    }
}

// Expression representing a function call
class FunctionCallExpr : MemberExpression
{
  Token name;
  ExprArray params;
  Function* fd;

  bool isVararg;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Identifier) && isNext(toks, TT.LeftParen);
    }

  // Read a parameter list (a,b,...)
  static ExprArray getParams(ref TokenArray toks)
    {
      ExprArray res;
      if(!isNext(toks, TT.LeftParen)) return res;

      Expression exp;

      // No parameters?
      if(isNext(toks, TT.RightParen)) return res;

      // Read the first parameter
      res ~= Expression.identify(toks);

      // Are there more?
      while(isNext(toks, TT.Comma))
        res ~= Expression.identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("Parameter list expected ')'", toks);

      return res;
    }

  void parse(ref TokenArray toks)
    {
      name = next(toks);
      loc = name.loc;

      params = getParams(toks);
    }

  char[] toString()
    {
      char[] result = name.str ~ "(";
      foreach(b; params)
	result ~= b.toString ~" ";
      return result ~ ")";
    }

  void resolve(Scope sc)
    {
      if(isMember) // Are we called as a member?
	{
          assert(leftScope !is null);
	  fd = leftScope.findFunc(name.str);
	  if(fd is null) fail(name.str ~ " is not a member function of "
		       ~ leftScope.toString, loc);
	}
      else
	{
          assert(leftScope is null);
	  fd = sc.findFunc(name.str);
	  if(fd is null) fail("Undefined function "~name.str, name.loc);
	}

      // Is this an idle function?
      if(fd.isIdle)
        {
          if(!sc.isStateCode)
            fail("Idle functions can only be called from state code",
                 name.loc);

          if(isMember)
            fail("Idle functions cannot be called as members", name.loc);
        }

      type = fd.type;
      assert(type !is null);

      isVararg = fd.isVararg;

      if(isVararg)
        {
          // The vararg parameter can match a variable number of
          // arguments, including zero.
          if(params.length < fd.params.length-1)
            fail(format("%s() expected at least %s parameters, got %s",
                        name.str, fd.params.length-1, params.length),
                 name.loc);
        }
      else
        // Non-vararg functions must match function parameter number
        // exactly
        if(params.length != fd.params.length)
          fail(format("%s() expected %s parameters, got %s",
                      name.str, fd.params.length, params.length),
               name.loc);

      // Check parameter types
      foreach(int i, par; fd.params)
	{
          // Handle varargs below
          if(isVararg && i == fd.params.length-1)
            break;

	  params[i].resolve(sc);
          try par.type.typeCast(params[i]);
	  catch(TypeException)
	    fail(format("%s() expected parameter %s to be type %s, not type %s",
                        name.str, i+1, par.type.toString, params[i].typeString),
                 name.loc);
	}

      // Loop through remaining arguments
      if(isVararg)
        {
          int start = fd.params.length-1;

          assert(fd.params[start].type.isArray);
          Type base = fd.params[start].type.getBase();

          foreach(int i, ref par; params[start..$])
            {
              par.resolve(sc);

              // If the first and last vararg parameter is of the
              // array type itself, then we are sending an actual
              // array. Treat it like a normal parameter.
              if(i == 0 && start == params.length-1 &&
                 par.type == fd.params[start].type)
                {
                  isVararg = false;
                  break;
                }

              // Otherwise, cast the type to the array base type.
              try base.typeCast(par);
              catch(TypeException)
                fail(format("Cannot convert %s of type %s to %s", par.toString,
                            par.typeString, base.toString), par.loc);
            }
        }
    }

  // Used in cases where the parameters need to be evaluated
  // separately from the function call. This is done by DotOperator,
  // in cases like obj.func(expr); Here expr is evaluated first, then
  // obj, and then finally the far function call. This is because the
  // far function call needs to read 'obj' off the top of the stack.
  void evalParams()
    {
      assert(pdone == false);

      foreach(i, ex; params)
        {
          ex.eval();

          // Convert 'const' parameters to actual constant references
          if(i < fd.params.length-1) // Skip the last parameter (in
                                     // case of vararg functions)
            if(fd.params[i].isConst)
              {
                assert(fd.params[i].type.isArray);
                tasm.makeArrayConst();
              }
        }

      if(isVararg)
        {
          // Compute the length of the vararg array.
          int len = params.length - fd.params.length + 1;

          // If it contains no elements, push a null array reference
          // (0 is always null).
          if(len == 0) tasm.push(0);
          else
            // Converte the pushed values to an array index
            tasm.popToArray(len, params[$-1].type.getSize());
        }

      // Handle the last parameter after everything has been
      // pushed. This will work for both normal and vararg parameters.
      if(fd.params.length != 0 && fd.params[$-1].isConst)
        {
          assert(fd.params[$-1].type.isArray);
          tasm.makeArrayConst();
        }

      pdone = true;
    }

  bool pdone;

  void evalAsm()
    {
      if(!pdone) evalParams();
      setLine();
      assert(fd.owner !is null);

      if(isMember)
        tasm.callFarFunc(fd.index, fd.owner.getIndex());
      else if(fd.isIdle)
        tasm.callIdle(fd.index, fd.owner.getIndex());
      else
        tasm.callFunc(fd.index, fd.owner.getIndex());
    }
}

// Expression that handles conversion from one type to another. This
// is used for implisit casts (eg. when using ints and floats
// together) and in the future it will also parse explisit casts. It
// acts as a wrapper that inserts the correct conversion code after an
// expression is compiled. The actual conversion code is done in the
// type itself. Compile time conversion of constants will also be done
// later. I am not sure how we will handle casts between objects yet -
// then need to be checked at runtime in the vm in any case.
class CastExpression : Expression
{
  // The original expression. The 'type' property in the base class
  // holds the new type.
  Expression orig;

  this(Expression exp, Type newType)
    {
      orig = exp;
      type = newType;

      assert(type !is null);
      assert(orig.type.canCastTo(type));
    }

  // These are only needed for explisit casts, and will be used when
  // "cast(Type)" expressions are implemented (if ever)
  void parse(ref TokenArray) { assert(0, "Cannot parse casts yet"); }
  void resolve(Scope sc) { assert(0, "Cannot resolve casts yet"); }

  bool isCTime()
    {
      return orig.isCTime() && orig.type.canCastCTime(type);
    }

  int[] evalCTime()
    {
      // Let the type do the conversion
      int[] res = type.typeCastCTime(orig);

      return res;      
    }

  void evalAsm()
    {
      orig.eval();

      // The type does the low-level stuff
      orig.type.evalCastTo(type);
    }

  char[] toString()
    {
      return "cast(" ~ type.toString ~ ")(" ~ orig.toString ~ ")";
    }
}
