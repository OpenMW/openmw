/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (operators.d) is part of the Monster script language
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

module monster.compiler.operators;

import monster.compiler.expression;
import monster.compiler.assembler;
import monster.compiler.tokenizer;
import monster.compiler.scopes;
import monster.compiler.types;
import monster.compiler.functions;
import monster.vm.error;
import monster.vm.arrays;

import std.stdio;

// Handles - ! ++ --
class UnaryOperator : Expression
{
  TT opType;
  Expression exp;
  bool postfix;

  this()
    {
      postfix = false;
    }

  // Used for postfix operators exp++ and exp--
  this(Expression ex, bool isAdd)
    {
      if(isAdd) opType = TT.PlusPlus;
      else opType = TT.MinusMinus;
      postfix = true;
      exp = ex;
    }

  static bool canParse(TokenArray toks)
    {
      return
	isNext(toks, TT.Minus) ||
	isNext(toks, TT.Not) ||
	isNext(toks, TT.PlusPlus) ||
	isNext(toks, TT.MinusMinus);
    }

  void parse(ref TokenArray toks)
    {
      opType = next(toks).type;
      exp = Expression.identifySub(toks);
    }

  char[] toString()
    {
      if(postfix) return "(" ~ exp.toString ~ ")" ~ tokenList[opType];
      return tokenList[opType] ~ "(" ~ exp.toString ~ ")";
    }

  // Copy everything from the sub expression
  void resolve(Scope sc)
    {
      exp.resolve(sc);

      type = exp.type;
      loc = exp.getLoc;

      if(opType == TT.PlusPlus || opType == TT.MinusMinus)
	{
          if(exp.isProperty)
            fail("Operators ++ and -- not implemented for properties yet",
                 loc);

	  if(!type.isIntegral)
	    fail("Operator " ~ tokenList[opType] ~ " not allowed for " ~
		 exp.toString() ~ " of type " ~ exp.typeString(), loc);

	  if(!exp.isLValue)
	    fail("Operator " ~ tokenList[opType] ~
		 " only allowed for lvalues, not " ~ exp.toString(),
		 loc);
	}

      if(opType == TT.Minus)
        {
          // Type check
          if(!type.isNumerical)
            fail("Unary minus only allowed for numerical types, not "
                 ~ exp.typeString, loc);

          // Convert unsigned types to signed when using unary minus
          if(type.isUint) type = BasicType.getInt();
          if(type.isUlong) type = BasicType.getLong();
        }

      if(opType == TT.Not && !type.isBool)
	fail("Boolean ! operator only allowed for bools, not "
             ~ exp.typeString, loc);
    }

  bool isCTime()
    {
      if(opType == TT.PlusPlus || opType == TT.MinusMinus)
	return false;

      return exp.isCTime();
    }

  int[] evalCTime()
    {
      int[] res = exp.evalCTime();

      assert(res.length == type.getSize());

      if(opType == TT.Minus)
        {
          if(type.isInt || type.isUint)
            res[0] *= -1;
          else if(type.isLong || type.isUlong)
            {
              long *lp = cast(long*)res.ptr;
              *lp *= -1;
            }
          else if(type.isFloat)
            {
              float *fp = cast(float*)res.ptr;
              *fp *= -1;
            }
          else if(type.isDouble)
            {
              double *fp = cast(double*)res.ptr;
              *fp *= -1;
            }
          else assert(0);
        }
      else fail("Cannot evaluate " ~ toString ~ " at compile time yet");

      return res;
    }

  void evalAsm()
    {
      if(opType == TT.PlusPlus || opType == TT.MinusMinus)
	{
	  exp.evalDest();

          assert(exp.type.isInt || exp.type.isUint ||
                 exp.type.isLong || exp.type.isUlong);

          setLine();
	  if(postfix)
	    {
	      if(opType == TT.PlusPlus) tasm.postInc(exp.type.getSize());
	      else tasm.postDec(exp.type.getSize());
	    }
	  else
	    {
	      if(opType == TT.PlusPlus) tasm.preInc(exp.type.getSize());
	      else tasm.preDec(exp.type.getSize());
	    }

          // The expression has been modified.
          exp.postWrite();

	  return;
	}

      exp.eval();

      setLine();

      if(opType == TT.Minus)
        {
          if(type.isInt || type.isLong || type.isFloat || type.isDouble)
            tasm.neg(type);
          // TODO: This is perhaps a little too strict, unsigned types
          // are not included
          else fail("unary minus not implemented for type " ~ typeString, loc);
        }
      else if(type.isBool && opType == TT.Not) tasm.not();
      else fail("unary operator " ~ toString() ~ " not implemented yet", loc);
    }
}

// OperatorExprs are compounds of expressions bound together by an
// operator. Eg. a+b is considered to be the single expression
// (a+b). Due to precedence condsiderations, these are not parsed in
// the normal way but rather put together from a list in
// Expression.identify(). Therefore, we do not need the parse()
// function.
abstract class OperatorExpr : Expression
{
  final void parse(ref TokenArray) { assert(0); }
}

// Any sub-expression with array operators suffixed, eg a[],
// getList()[14], and array[2..$];
class ArrayOperator : OperatorExpr
{
  bool isSlice;

  // name[], name[index], name[index..index2]
  Expression name, index, index2;

  this(Expression name, Floc loc,
       Expression index = null,
       Expression index2 = null)
    {
      this.name = name;
      this.index = index;
      this.index2 = index2;
      this.loc = loc;
    }

  char[] toString()
    {
      char[] res = "(" ~ name.toString ~ "[";
      if(index !is null)
	res ~= index.toString;
      if(index2 !is null)
	res ~= " .. " ~ index2.toString;
      return res ~= "])";
    }

  // We can ALWAYS assign to elements of an array. There is no such
  // thing as a constant array in Monster, only constant array
  // references. Exceptions (such as strings in the static data area)
  // must be dealt with at runtime. We might moderate this rule later
  // though.
  bool isLValue() { return true; }

  void resolve(Scope sc)
    {
      name.resolve(sc);

      // Copy the type of the name expression
      type = name.type;

      // Check that we are indeed an array.
      if(!type.isArray)
	fail("Expression '" ~ name.toString ~ "' of type '" ~ name.typeString
	     ~ "' is not an array", loc);

      // If we have one and only one index expression i[4], reduce the
      // array level by one, but not if we have two.
      if(index !is null)
        {
          // The first index is present. Our total expression is
          // either name[index] or name[index..index2]

          // Create an inner scope where $ is a valid character.
          ArrayScope isc = new ArrayScope(sc, name);

          index.resolve(isc);

          // The indices must be ints
          Type tpint = BasicType.getInt();
          try tpint.typeCast(index);
          catch(TypeException)
            fail("Cannot convert array index " ~ index.toString ~ " to int");

          if(index2 !is null)
            {
              // slice, name[index..index2]
              index2.resolve(isc);
              try tpint.typeCast(index2);
              catch(TypeException)
                fail("Cannot convert array index " ~ index2.toString ~ " to int");
              isSlice = true;
            }
          else
            // element access, name[index]. Change the type to the
            // element type.
            type = type.getBase();
        }
      // Complete slice, a[]
      else isSlice = true;
    }

  bool isCTime()
    {
      if(isDest) return false;

      // a[b] and a[b..c]; is compile time if a, b and c is.
      if(index !is null && !index.isCTime) return false;
      if(index2 !is null && !index2.isCTime) return false;
      return name.isCTime();
    }

  AIndex arrind;

  int[] evalCTime()
    {
      // Get the array index
      int[] arr = name.evalCTime();
      assert(arr.length == 1);

      // Full slice, a[]. No extra action is needed.
      if(index is null) return arr;

      ArrayRef *arf = arrays.getRef(cast(AIndex)arr[0]);

      int elem = arf.elemSize;
      assert(elem == name.type.getBase().getSize());

      // Get the first index
      arr = index.evalCTime();
      assert(arr.length == 1);
      int ind = arr[0];

      if(index2 is null)
        {
          // Normal indexing.
          arr = arf.iarr[ind*elem..(ind+1)*elem];
          assert(arr.length == elem);
          return arr;
        }

      // Slice a[b..c]. Get the second index.
      arr = index2.evalCTime();
      assert(arr.length == 1);
      int ind2 = arr[0];

      // Create a new array ref as a constant slice of the original
      // data, and return it
      arf = arrays.createConst(arf.iarr[ind*elem..ind2*elem], elem);
      arrind = arf.getIndex();
      return (cast(int*)&arrind)[0..1];
    }

  // Since very little separates eval() and evalDest(), use a shared
  // function.
  bool isDest = false;

  void evalAsm()
    {
      // Push the array index first
      name.eval();

      setLine();

      if(index is null)
        {
          // Full slice, a[]. No extra action is needed.
          assert(index2 is null);
          assert(isSlice);
        }

      else if(index2 is null)
        {
          // Normal indexing, eg array[2]

          // Push the index, and then go fetch the element or push a
          // pointer that directs us to the array.
          index.eval();

          setLine();
          if(isDest) tasm.elementAddr();
          else tasm.fetchElement(/*type.getSize*/);

          assert(!isSlice);
        }
      else
        {
          // Slice, eg. array[1..4]

          // Push the indices
          index.eval();
          index2.eval();

          setLine();
          tasm.makeSlice();
        }
    }

  void evalDest()
    {
      isDest = true;
      eval();
    }
}

// Handles the DOT operator, eg. someObj.someFunc();
class DotOperator : OperatorExpr
{
  // owner.member
  Expression owner, member;

  this(Expression own, Expression memb, Floc loc)
    {
      this.owner = own;

      assert(own !is null, "owner cannot be null");
      assert(memb !is null);

      this.member = memb;

      this.loc = loc;
    }

  char[] toString()
    {
      return "(" ~ owner.toString ~ "." ~ member.toString ~ ")";
    }

  bool isLValue()
    {
      // For dots, the right hand side must be an lvalue. The left
      // side can be anything, it just specifies where the right side
      // is looked up.
      return member.isLValue;
    }

  // We are a property if the member is a property
  bool isProperty() { return member.isProperty; }
  void writeProperty() { member.writeProperty(); }

  void resolve(Scope sc)
    {
      // In order to find the correct scope for the right side, we
      // need to resolve and find the type of the left side first.
      owner.resolve(sc);

      Type ot = owner.type;

      if(ot.getMemberScope() is null)
        fail(owner.toString() ~ " of type " ~ owner.typeString()
             ~ " cannot have members", loc);

      // Resolve the member in the context of the left scope.
      member.resolveMember(sc, ot);

      type = member.type;
    }

  // TODO: An evalMemberCTime() function could be used here

  void evalAsm()
    {
      evalCommon();
      member.eval();
    }

  void evalDest()
    {
      evalCommon();
      member.evalDest();
    }

  void postWrite() { member.postWrite(); }

  void evalCommon()
    {
      // If the the expression is a function call, we must push the
      // parameters first
      auto fc = cast(FunctionCallExpr)member;
      if(fc !is null)
	fc.evalParams();

      // Ask the rhs if it needs the lhs. It says no if the rhs is
      // staic or evaluatable at compile time, eg a type name, part of
      // an enum, a static function or a static property. So if you
      // use int i; i.max, you could get int.max without evaluating i.
      if(!member.isStatic)
        // We push the owner up front and let the member take care of
        // it.
        owner.eval();
    }
}

// Assignment operators, =, +=, *=, /=, %=, ~=
class AssignOperator : BinaryOperator
{
  // Set to true if we are assigning to an array slice, eg.  a[1..3] =
  // b[] or a[] = 3; This changes some rules, eg array data must be
  // copied instead or just their reference, and we are allowed to
  // assign a single value to a slice and fill the array that way.
  bool isSlice;
  bool isFill; // Used for filling elements with one value.

  bool catElem; // For concatinations (~=), true when the right hand
                // side is a single element rather than an array.

  this(Expression left, Expression right, TT opType, Floc loc)
    { super(left, right, opType, loc); }

  void resolve(Scope sc)
    {
      left.resolve(sc);
      right.resolve(sc);

      // The final type is always from the left expression
      type = left.type;

      // Check that we are allowed assignment
      if(!left.isLValue)
	fail("Cannot assign to expression '" ~ left.toString ~ "'", loc);

      // Operators other than = and ~= are only allowed for numerical
      // types.
      if(opType != TT.Equals && opType != TT.CatEq && !type.isNumerical)
	fail("Assignment " ~tokenList[opType] ~
	     " not allowed for non-numerical type " ~ typeString(), loc);

      // Is the left hand expression a sliced array?
      auto arr = cast(ArrayOperator) left;
      if(arr !is null && arr.isSlice)
        {
          assert(type.isArray);
          isSlice = true;

          if(opType == TT.CatEq)
            fail("Cannot use ~= on array slice " ~ left.toString, loc);

          // For array slices on the right hand side, the left hand
          // type must macth exactly, without implisit casting. For
          // example, the following is not allowed, even though 3 can
          // implicitly be cast to char[]:
          // char[] a;
          // a[] = 3; // Error

          // We are, on the other hand, allowed to assign a single
          // value to a slice, eg:
          // int[] i = new int[5];
          // i[] = 3; // Set all elements to 3.
          // In this case we ARE allowed to typecast, though.

          if(right.type == left.type) return;

          Type base = type.getBase();

          try base.typeCast(right);
          catch(TypeException)
            fail("Cannot assign " ~ right.toString ~ " of type " ~ right.typeString
                 ~ " to slice " ~ left.toString ~ " of type "
                 ~ left.typeString, loc);

          // Inform eval() what to do
          isFill = true;

          return;
        }

      // Handle concatination ~=
      if(opType == TT.CatEq)
        {
          if(!left.type.isArray)
            fail("Opertaor ~= can only be used on arrays, not " ~ left.toString ~
                 " of type " ~ left.typeString, left.loc);

          // Array with array
          if(left.type == right.type) catElem = false;
          // Array with element
          else if(right.type.canCastOrEqual(left.type.getBase()))
            {
              left.type.getBase().typeCast(right);
              catElem = true;
            }
          else
            fail("Cannot use operator ~= on types " ~ left.typeString ~
                 " and " ~ right.typeString, left.loc);
          return;
        }

      // Cast the right side to the left type, if possible.
      try type.typeCast(right);
      catch(TypeException)
	fail("Assignment " ~tokenList[opType] ~ ": cannot implicitly cast " ~
	     right.typeString() ~ " to " ~ left.typeString(), loc);

      assert(left.type == right.type);
    }

  void evalAsm()
    {
      int s = right.type.getSize;

      // += -= etc are implemented without adding new
      // instructions. This might change later. The "downside" to this
      // approach is that the left side is evaluated two times, which
      // might not matter much for lvalues anyway, but it does give
      // more M-code vs native code, and thus more overhead. I'm not
      // sure if a function call can ever be an lvalue or if lvalues
      // can otherwise have side effects in the future. If that
      // becomes the case, then this implementation will have to
      // change. Right now, the expression i += 3 will be exactly
      // equivalent to i = i + 3.

      if(opType != TT.Equals)
	{
	  // Get the values of both sides
	  left.eval();
	  right.eval();

          setLine();

          // Concatination
          if(opType == TT.CatEq)
            {
              assert(type.isArray);

              if(catElem)
                // Append one element onto the array
                tasm.catArrayRight(s);
              else
                // Concatinate two arrays.
                tasm.catArray();
            }

	  // Perform the arithmetic operation. This puts the result of
	  // the addition on the stack. The evalDest() and mov() below
	  // will store it in the right place.
	  else if(type.isNumerical)
	    {
	      if(opType == TT.PlusEq) tasm.add(type);
	      else if(opType == TT.MinusEq) tasm.sub(type);
	      else if(opType == TT.MultEq) tasm.mul(type);
	      else if(opType == TT.DivEq) tasm.div(type);
	      else if(opType == TT.IDivEq) tasm.idiv(type);
	      else if(opType == TT.RemEq) tasm.divrem(type);
	      else fail("Unhandled assignment operator", loc);
	    }
	  else assert(0, "Type not handled");
	}
      else right.eval();

      // Special case for properties and other cases where the
      // assignment is actually a function call. We don't call
      // evalDest or mov, just let writeProperty handle everything.
      if(left.isProperty)
        {
          left.writeProperty();
          return;
        }

      left.evalDest();

      setLine();

      // Special case for left hand slices, of the type a[] or
      // a[i..j].
      if(isSlice)
        {
          assert(type.isArray);

          if(isFill)
            {
              // Fill the array with the result of the right-hand
              // expression
              tasm.fillArray(s);
            }
          else
            // Copy array contents from the right array to the left
            // array
            tasm.copyArray();

          return;
        }

      tasm.mov(right.type.getSize()); // Move the data

      // Left hand value has been modified, notify it.
      left.postWrite();
    }
}

// Boolean operators: ==, !=, <, >, <=, >=, &&, ||, =i=, =I=, !=i=, !=I=
class BooleanOperator : BinaryOperator
{
  this(Expression left, Expression right, TT opType, Floc loc)
    { super(left, right, opType, loc); }

  void resolve(Scope sc)
    {
      left.resolve(sc);
      right.resolve(sc);

      // Cast to a common type
      try Type.castCommon(left, right);
      catch(TypeException)
	fail("Boolean operator " ~tokenList[opType] ~ " not allowed for types " ~
	     left.typeString() ~ " and " ~ right.typeString(), loc);

      // At this point the types must match
      assert(left.type == right.type);

      type = BasicType.getBool;

      // TODO: We might allow < and > for strings at some point.
      if(opType == TT.Less || opType == TT.More || opType == TT.LessEq ||
	 opType == TT.MoreEq)
        {
          if(!left.type.isNumerical() && !left.type.isChar())
            fail("Cannot use operator " ~ tokenList[opType] ~ " for type "
                 ~ left.typeString(), loc);
        }
      else if(opType == TT.And || opType == TT.Or)
        {
          if(!left.type.isBool())
            fail("Operator " ~ tokenList[opType] ~ " cannot be used on type " ~
                 left.typeString(), loc);
        }
      else
        {
          assert(opType == TT.IsEqual || opType == TT.NotEqual ||
                 opType == TT.IsCaseEqual || opType == TT.NotCaseEqual);

          // Nested arrays are handled recursively and element per
          // element. This is not implemented yet.
          if(left.type.arrays > 1)
            fail("Boolean operators do not yet support nested arrays", loc);

          if(opType == TT.IsCaseEqual || opType == TT.NotCaseEqual)
            if(!left.type.isChar() && !left.type.isString)
              fail("Cannot use case insensitive operator " ~ tokenList[opType]  ~
                   " on type " ~ left.typeString(), loc);
        }
    }

  // Pretty much all of these can be evaluated at compile time when
  // both sides are ctime. In the cases of && and ||, it's enough that
  // one side is ctime to optimize the code, and sometimes cases make
  // the expression compile time. The optimization cases must be
  // handled be handled in evalAsm, since they are not applicable to
  // static initialization.
  bool isCTime()
    {
      // Only compute && and || for now, add the rest later.
      if(!(opType == TT.And || opType == TT.Or)) return false;

      if(left.isCTime && right.isCTime) return true;

      // Handle shortened cases
      if(opType == TT.Or)
        // If the left side is compile time computable and is true,
        // the entire value must be true. The same is true for the
        // right side, but the user expects the left-most expression
        // to be evaluated at runtime, so we must do that.
        if(leftIs(true)) return true;

      if(opType == TT.And)
        // If the first side is false, we are a compile time value.
        if(leftIs(false)) return true;

      return false;
    }

  private
    {
      bool leftIs(bool t)
        {
          if(!left.isCTime) return false;
          int[] val = left.evalCTime();
          assert(val.length == 1);
          if(val[0]) return t;
          else return !t;
        }
      bool rightIs(bool t)
        {
          if(!right.isCTime) return false;
          int[] val = right.evalCTime();
          assert(val.length == 1);
          if(val[0]) return t;
          else return !t;
        }

      bool ctimeval;
    }

  int[] evalCTime()
    {
      if(opType == TT.And)
        ctimeval = !(leftIs(false) || rightIs(false));
      else if(opType == TT.Or)
        ctimeval = (leftIs(true) || rightIs(true));

      return (cast(int*)&ctimeval)[0..1];
    }

  void evalAsm()
    {
      bool negate = false;
      int label;
      Type tp = left.type;
      int s = tp.getSize();

      // To reduce the number of operations we implement, we treat <
      // and > the same, but just reverse the order of expressions, so
      // eg. a > b gets treated the same as b < a.

      if(opType == TT.More || opType == TT.LessEq)
	{
	  right.eval();
	  left.eval();
	}
      else if(opType == TT.And || opType == TT.Or)
	{
          assert(tp.isBool);

          // TODO: We do compile time shortening here, if possible,
          // and eliminate unneccesary code.

          // && and || are 'shortened', so even in the runtime case we
          // only evaluate the first operand here.
	  left.eval();
	}
      else
	{
	  left.eval();
	  right.eval();
	}

      setLine();

      // Treat array types separately
      if(tp.arrays != 0)
        {
          if(tp.arrays != 1)
            fail("Cannot compare nested arrays yet", loc);
          assert(s == 1);

          switch(opType)
            {
            case TT.NotEqual:
              negate = true;
            case TT.IsEqual:
              tasm.cmpArray();
              break;

            case TT.NotCaseEqual:
              negate = true;
            case TT.IsCaseEqual:
              assert(tp.isString, "Should not get here on non-string types");
              tasm.icmpString();
              break;
            }
          if(negate) tasm.not();
          return;
        }

      // Non-array types
      switch(opType)
	{
	case TT.NotEqual:
          negate = true;
	case TT.IsEqual:
	  tasm.isEqual(s);
	  break;

        case TT.NotCaseEqual:
          negate = true;
        case TT.IsCaseEqual:
          assert(s == 1 && tp.isChar);
          tasm.isCaseEqual();
          break;

	case TT.LessEq:
	case TT.MoreEq:
          negate=true;
	case TT.Less:
	case TT.More:
          tasm.less(tp);
	  break;

	case TT.And:
	  // Skip the second operand if the first is false. The
	  // implementation here might be optimized later.
	  tasm.dup();
	  label = tasm.jumpz();
	  tasm.pop();
	  right.eval(); // The second value now determines the final
			// value of the expression.
	  tasm.label(label);
	  break;

	case TT.Or:
	  tasm.dup();
	  label = tasm.jumpnz();
	  tasm.pop();
	  right.eval();
	  tasm.label(label);
	  break;

	default:
	  fail("boolean operator "~tokenList[opType]~" not implemented", loc);
	}

      // These not's can in many cases be optimized away at a later
      // stage by peephole optimization. For example, a not and a
      // jumpz is converted to a jumpnz. We might also create a
      // isNotEqual instruction later on if that will significantly
      // improve performance.
      if(negate) tasm.not();
    }
}

extern(C) double floor(double d);

// All other binary operators.
class BinaryOperator : OperatorExpr
{
  Expression left, right;
  TT opType;

  // Used with ~, is one side an element (instead of an array)?
  enum CatElem { None, Left, Right }
  CatElem cat;

  this(Expression left, Expression right, TT opType, Floc loc)
    {
      this.left = left;
      this.right = right;
      this.opType = opType;
      this.loc = loc;
    }

  char[] toString()
    {
      char[] opName = tokenList[opType];
      return "(" ~ left.toString ~ opName ~ right.toString ~ ")";
    }

  void resolve(Scope sc)
    {
      left.resolve(sc);
      right.resolve(sc);
      cat = CatElem.None;

      // Special case for concatination
      if(opType == TT.Cat)
        {
          // Array with array
          if(left.type.isArray && left.type == right.type)
            {
              type = left.type;
              cat = CatElem.None;
              return;
            }

          // Element with array?
          if(right.type.isArray && left.type.canCastOrEqual(right.type.getBase()))
            {
              right.type.getBase().typeCast(left);
              type = right.type;
              cat = CatElem.Left;
              return;
            }

          // Array with element?
          if(left.type.isArray && right.type.canCastOrEqual(left.type.getBase()))
            {
              left.type.getBase().typeCast(right);
              type = left.type;
              cat = CatElem.Right;
              return;
            }

          fail("Cannot use operator ~ on types " ~ left.typeString ~
               " and " ~ right.typeString, left.loc);
        }

      // Assume arithmetic operator for now (Later, check the complete
      // list in tokenizer and Expression.identify and treat every
      // case correctly.)

      // Cast to a common type
      bool doFail = false;
      try Type.castCommon(left, right);
      catch(TypeException)
        doFail = true;

      type = right.type;

      if(!type.isNumerical || doFail)
	fail("Operator " ~tokenList[opType] ~ " not allowed for types " ~
	     left.typeString() ~ " and " ~ right.typeString(), loc);

      // At this point the types must match
      assert(left.type == right.type);
    }

  bool isCTime()
    {
      return
        left.isCTime() && right.isCTime() &&
        opType != TT.Cat;

      // We do not handle concatination here, because this operation
      // is expected to create a new array each time it is executed,
      // and the array is expected to be dynamic. We could still
      // optimize this using compile time values, but it's not a
      // priority.
    }

  int[2] ctimeval;

  int[] evalCTime()
    {
      int ls[] = left.evalCTime();
      int rs[] = right.evalCTime();
      assert(ls.length == left.type.getSize());
      assert(rs.length == right.type.getSize());

      assert(type.isNumerical);

      if(type.isInt || type.isUint)
        {
          int v1 = ls[0];
          int v2 = rs[0];
          int res;
          switch(opType)
            {
            case TT.Plus: res = v1+v2; break;
            case TT.Minus: res = v1-v2; break;
            case TT.Mult: res = v1*v2; break;
            case TT.Div:
            case TT.IDiv:
              if(type.isInt)
                res = v1/v2;
              else
                res = (cast(uint)v1)/(cast(uint)v2);
              break;
            case TT.Rem:
              if(type.isInt)
                res = v1%v2;
              else
                res = (cast(uint)v1)%(cast(uint)v2);
              break;
            }
          ctimeval[0] = res;
        }
      else if(type.isLong || type.isUlong)
        {
          long v1 = *(cast(long*)ls.ptr);
          long v2 = *(cast(long*)rs.ptr);
          long res;
          switch(opType)
            {
            case TT.Plus: res = v1+v2; break;
            case TT.Minus: res = v1-v2; break;
            case TT.Mult: res = v1*v2; break;
            case TT.Div:
            case TT.IDiv:
              if(type.isLong)
                res = v1/v2;
              else
                res = (cast(ulong)v1)/(cast(ulong)v2);
              break;
            case TT.Rem:
              if(type.isLong)
                res = v1%v2;
              else
                res = (cast(ulong)v1)%(cast(ulong)v2);
              break;
            }
          *(cast(long*)ctimeval.ptr) = res;
        }
      else if(type.isFloat)
        {
          float v1 = *(cast(float*)ls.ptr);
          float v2 = *(cast(float*)rs.ptr);
          float res;
          switch(opType)
            {
            case TT.Plus: res = v1+v2; break;
            case TT.Minus: res = v1-v2; break;
            case TT.Mult: res = v1*v2; break;
            case TT.Div: res = v1/v2; break;
            case TT.IDiv: res = floor(v1/v2); break;
            case TT.Rem:
              res = v1-v2*floor(v1/v2);
              break;
            }
          *(cast(float*)ctimeval.ptr) = res;
        }
      else if(type.isDouble)
        {
          double v1 = *(cast(double*)ls.ptr);
          double v2 = *(cast(double*)rs.ptr);
          double res;
          switch(opType)
            {
            case TT.Plus: res = v1+v2; break;
            case TT.Minus: res = v1-v2; break;
            case TT.Mult: res = v1*v2; break;
            case TT.Div: res = v1/v2; break;
            case TT.IDiv: res = floor(v1/v2); break;
            case TT.Rem:
              res = v1-v2*floor(v1/v2);
              break;
            }
          *(cast(double*)ctimeval.ptr) = res;
        }

      // Return the final value
      assert(type.getSize <= 2);
      return ctimeval[0..type.getSize()];
    }

  void evalAsm()
    {
      left.eval();
      right.eval();

      setLine();

      // Concatination
      if(opType == TT.Cat)
        {
          switch(cat)
            {
            case CatElem.None:
              // Concatinating two arrays.
              tasm.catArray();
              break;
            case CatElem.Left:
              tasm.catArrayLeft(left.type.getSize);
              break;
            case CatElem.Right:
              tasm.catArrayRight(right.type.getSize);
              break;
            default:
              assert(0, "Illegal value of 'cat'");
            }
          return;
        }

      // All other operators

      // If any type conversion is necessary to perform the operator,
      // it has already been taken care of in resolve().
      assert(left.type == right.type, "Type mismatch: " ~
	     left.type.toString ~ " != " ~ right.type.toString);

      if(type.isNumerical)
	{
	  if(opType == TT.Plus) tasm.add(type);
	  else if(opType == TT.Minus) tasm.sub(type);
	  else if(opType == TT.Mult) tasm.mul(type);
	  else if(opType == TT.Div) tasm.div(type);
	  else if(opType == TT.IDiv) tasm.idiv(type);
	  else if(opType == TT.Rem) tasm.divrem(type);
	  else fail("operator "~tokenList[opType]~
		    " not implemented yet");
	}
      else assert(0, "Binary operators not implemented for this type yet: " ~ typeString);
    }
}
