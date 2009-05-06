/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
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
import monster.compiler.enums;
import monster.compiler.variables;
import monster.vm.error;
import monster.vm.arrays;

import std.stdio;
import std.string;
import std.utf;

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
          setLine();
          exp.incDec(opType, postfix);
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
  bool isSlice; // Set if this is a slice
  bool isFill;  // Set during assignment if we're filling the array
                // with one single value

  int isEnum;  // Used for enum types

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

  // We can ALWAYS assign to elements of an array. Const arrays are
  // handled at runtime only at the moment.
  bool isLValue() { return true; }

  void resolve(Scope sc)
    {
      name.resolve(sc);

      // Copy the type of the name expression
      type = name.type;

      // May be used on enums as well
      if(type.isEnum ||
         (type.isMeta && type.getBase.isEnum))
        {
          if(type.isMeta)
            type = type.getBase();

          assert(type.isEnum);

          if(index is null)
            fail("Missing lookup value", loc);

          if(index2 !is null)
            fail("Slices are not allowed for enums", loc);

          index.resolve(sc);

          // The indices must be ints
          Type lng = BasicType.getLong();
          if(index.type.canCastTo(lng))
            {
              lng.typeCast(index, "");
              isEnum = 1;
            }
          else
            {
              if(!index.type.isString)
                fail("Enum lookup value must be a number or a string, not type "
                     ~index.typeString, loc);
              isEnum = 2;
            }

          return;
        }

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
          tpint.typeCast(index, "array index");

          if(index2 !is null)
            {
              // slice, name[index..index2]
              index2.resolve(isc);
              tpint.typeCast(index2, "array index");
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
      if(isEnum) return index.isCTime;

      if(isDest) return false;

      // a[b] and a[b..c]; is compile time if a, b and c is.
      if(index !is null && !index.isCTime) return false;
      if(index2 !is null && !index2.isCTime) return false;
      return name.isCTime();
    }

  AIndex arrind;

  int[] evalCTime()
    {
      if(isEnum)
        {
          assert(index !is null && index2 is null);
          auto et = cast(EnumType)type;
          assert(et !is null);
          EnumEntry *ptr;

          if(isEnum == 1)
            {
              long val = *(cast(long*)index.evalCTime().ptr);
              ptr = et.lookup(val);
              if(ptr is null)
                fail("No matching value " ~ .toString(val) ~ " in enum", loc);
            }
          else
            {
              assert(isEnum == 2);
              AIndex ai = cast(AIndex)index.evalCTime()[0];
              char[] str = toUTF8(arrays.getRef(ai).carr);
              ptr = et.lookup(str);
              if(ptr is null)
                fail("No matching value " ~ str ~ " in enum", loc);
            }
          return (&(ptr.index))[0..1];
        }

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
      if(isEnum)
        {
          assert(index !is null && index2 is null);
          assert(type.isEnum);
          assert((isEnum == 1 && index.type.isLong) ||
                 (isEnum == 2 && index.type.isString));

          // All we need is the index value
          index.eval();

          if(isEnum == 1)
            tasm.enumValToIndex(type.tIndex);
          else
            tasm.enumNameToIndex(type.tIndex);
          return;
        }

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
          else tasm.fetchElement();

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

          assert(isSlice);
        }
    }

  void evalDest()
    {
      assert(isLValue);
      isDest = true;
      eval();
    }

  void store()
    {
      evalDest();

      if(isSlice)
        {
          // Special case for left hand slices, of the type a[] or
          // a[i..j]. In this case we use special commands to fill or
          // copy the entire array data.

          if(isFill)
            tasm.fillArray(type.getBase().getSize());
          else tasm.copyArray();
        }
      else
        tasm.mov(type.getSize());
    }
}

// Handles the DOT operator, eg. someObj.someFunc();
class DotOperator : OperatorExpr
{
  // owner.member
  Expression owner;
  MemberExpr member;

  this(Expression own, Expression memb, Floc loc)
    {
      owner = own;

      assert(own !is null, "owner cannot be null");
      assert(memb !is null);

      member = cast(MemberExpr)memb;
      assert(member !is null);

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

  void resolve(Scope sc)
    {
      // In order to find the correct scope for the right side, we
      // need to resolve and find the type of the left side first.
      owner.resolve(sc);

      Type ot = owner.type;
      assert(ot !is null, "owner " ~ owner.toString ~ " has no type!");

      ot.getMemberScope();

      if(ot.getMemberScope() is null)
        fail(owner.toString() ~ " of type " ~ owner.typeString()
             ~ " cannot have members", loc);

      // Resolve the member in the context of the left scope.
      member.resolveMember(sc, ot);

      type = member.type;

      // Make sure we only call static members when the owner is a
      // type.
      if(ot.isMeta && !member.isStatic)
        fail("Can only access static members for " ~ owner.toString,
             loc);
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

  void store()
    {
      evalCommon();
      member.store();
    }

  void incDec(TT o, bool b)
    {
      evalCommon();
      member.incDec(o,b);
    }

  void evalCommon()
    {
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

// Boolean operators: ==, !=, <, >, <=, >=, &&, ||, =i=, =I=, !=i=, !=I=
class BooleanOperator : BinaryOperator
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

  bool isMeta = false;

 public:

  this(Expression left, Expression right, TT opType, Floc loc)
    { super(left, right, opType, loc); }

 override:

  void resolve(Scope sc)
    {
      left.resolve(sc);
      right.resolve(sc);

      type = BasicType.getBool;

      // Check if one or both types are meta-types first
      if(left.type.isMeta || right.type.isMeta)
        {
          isMeta = true;

          if(opType != TT.IsEqual && opType != TT.NotEqual)
            fail("Cannot use operator " ~ tokenList[opType] ~ " on types", loc);

          assert(isCTime());

          // This means we have one of the following cases:
          // i == int
          // int == int

          // In these cases, we compare the types only, and ignore the
          // values of any expressions (they are not computed.)

          // Get base types
          Type lb = left.type;
          Type rb = right.type;
          if(lb.isMeta) lb = lb.getBase();
          if(rb.isMeta) rb = rb.getBase();

          // Compare the base types and store the result
          ctimeval = (lb == rb) != 0;

          if(opType == TT.NotEqual)
            ctimeval = !ctimeval;
          else
            assert(opType == TT.IsEqual);

          return;
        }

      // Cast to a common type
      Type.castCommon(left, right);

      // At this point the types must match
      assert(left.type == right.type);

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
      // If both types are meta-types, then we might be ctime.
      if(isMeta)
        return (opType == TT.IsEqual || opType == TT.NotEqual);

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

  int[] evalCTime()
    {
      if(opType == TT.And)
        ctimeval = !(leftIs(false) || rightIs(false));
      else if(opType == TT.Or)
        ctimeval = (leftIs(true) || rightIs(true));
      else
        // For meta types, ctimeval is already set
        assert(isMeta);

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
              right.type.getBase().typeCast(left, "");
              type = right.type;
              cat = CatElem.Left;
              return;
            }

          // Array with element?
          if(left.type.isArray && right.type.canCastOrEqual(left.type.getBase()))
            {
              left.type.getBase().typeCast(right, "");
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
      Type.castCommon(left, right);

      type = right.type;

      if(!type.isNumerical)
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
