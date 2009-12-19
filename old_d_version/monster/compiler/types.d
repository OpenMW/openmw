/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (types.d) is part of the Monster script language package.

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

module monster.compiler.types;

import monster.compiler.tokenizer;
import monster.compiler.enums;
import monster.compiler.scopes;
import monster.compiler.expression;
import monster.compiler.assembler;
import monster.compiler.properties;
import monster.compiler.block;
import monster.compiler.functions;
import monster.compiler.variables;
import monster.compiler.states;
import monster.compiler.structs;
import monster.vm.arrays;
import monster.vm.mclass;
import monster.vm.mobject;
import monster.vm.error;

import monster.options;

import std.stdio;
import std.utf;
import std.string;

// A class that represents a type. The Type class is abstract, and the
// different types are actually handled by various subclasses of Type
// (see below.) The static function identify() is used to figure out
// exactly which subclass to use.
abstract class Type : Block
{
  // Can the given tokens possibly be parsed as a type? This is not
  // meant as an extensive test to differentiate between types and
  // non-types, it is more like a short-cut to get the type tokens out
  // of the way. It should only be called from places where you
  // require a type (and only in canParse(), since it removes the
  // tokens.)
  static bool canParseRem(ref TokenArray toks)
    {
      // This function is a bit messy unfortunately. We should improve
      // the process so we can clean it up. Preferably we should
      // rewrite the entire parser.

      // We allow typeof(expression) as a type
      if(isNext(toks, TT.Typeof))
        {
          skipParens(toks);
          return true;
        }

      // The 'var' keyword can be used as a type
      if(isNext(toks, TT.Var)) return true;

      if(FuncRefType.canParseRem(toks))
        return true;

      // Allow typename
      if(!isNext(toks, TT.Identifier)) return false;

      // Allow a.b.c
      while(isNext(toks, TT.Dot))
        if(!isNext(toks, TT.Identifier)) return false;

      // Allow a[][]
      while(isNext(toks,TT.LeftSquare))
	if(!isNext(toks,TT.RightSquare))
	  return false;

      return true;
    }

  // Parse a type specifieer and return the correct class to handle it
  // (fully parsed). Currently valid type formats are
  //
  // identifier - either a basic type (int, bool, etc) or a class name
  // identifier[] - array
  // identifier[][]... - arrays can be multi-dimensional
  //
  // static buffers, ie. int[10] are not allowed as types. The only
  // place a type is allowed to take array expressions is after a new,
  // eg. int[] i = new int[10], in that case you should set the second
  // parameter to true. The expression array is stored in exps.
  static Type identify(ref TokenArray toks, bool takeExpr, ref ExprArray exps)
    {
      assert(toks.length != 0);

      // Model this after Exp.idSub or Code.identify
      Type t = null;

      // Find what kind of type this is and create an instance of the
      // corresponding class. TODO: Lots of redundant objects created
      // here.
      if(BasicType.canParse(toks)) t = new BasicType();
      else if(UserType.canParse(toks)) t = new UserType();
      else if(GenericType.canParse(toks)) t = new GenericType();
      else if(TypeofType.canParse(toks)) t = new TypeofType();
      else if(FuncRefType.canParse(toks)) t = new FuncRefType();
      else fail("Cannot parse " ~ toks[0].str ~ " as a type", toks[0].loc);

      // Parse the actual tokens with our new and shiny object.
      t.parse(toks);

      // Add arrays here afterwards by wrapping the previous type in
      // an ArrayType.
      exps = VarDeclaration.getArray(toks, takeExpr);

      if(t.isVar && exps.length)
        fail("Cannot have arrays of var (yet)", t.loc);

      // Despite looking strange, this code is correct.
      foreach(e; exps)
        t = new ArrayType(t);

      return t;
    }

  // Short version of the above, when expressions are not allowed.
  static Type identify(ref TokenArray toks)
    {
      ExprArray exp;
      return identify(toks, false, exp);
    }

  // Lookup table of all types
  static Type[int] typeList;
  int tIndex; // Index of this type

  // The complete type name including specifiers, eg. "int[]".
  char[] name;

  MetaType meta;
  final MetaType getMeta()
    {
      if(meta is null)
        meta = new MetaType(this);
      return meta;
    }

  this()
    {
      tIndex = typeList.length;
      typeList[tIndex] = this;
      name = "UNNAMED TYPE";
    }

  // Used for easy checking
  bool isInt() { return false; }
  bool isUint() { return false; }
  bool isLong() { return false; }
  bool isUlong() { return false; }
  bool isChar() { return false; }
  bool isBool() { return false; }
  bool isFloat() { return false; }
  bool isDouble() { return false; }
  bool isVoid() { return false; }

  bool isVar() { return false; }

  bool isString() { return false; }

  bool isArray() { return arrays() != 0; }
  bool isObject() { return false; }
  bool isPackage() { return false; }
  bool isStruct() { return false; }
  bool isEnum() { return false; }
  bool isIntFunc() { return false; }
  bool isFuncRef() { return false; }

  bool isReplacer() { return false; }

  bool isIntegral() { return isInt || isUint || isLong || isUlong; }
  bool isFloating() { return isFloat || isDouble; }

  // Numerical types allow arithmetic operators
  bool isNumerical() { return isIntegral() || isFloating(); }

  // Is this a meta-type? A meta-type is the type of expressions like
  // 'int' and 'ClassName' - they themselves describe a type.
  // Meta-types always have a base type accessible through getBase(),
  // and a member scope similar to variables of the type itself.
  bool isMeta() { return false; }

  // Is this a legal type for variables? If this is false, you cannot
  // create variables of this type, neither directly or indirectly
  // (through automatic type inference.)
  bool isLegal() { return true; }

  // Get base type (used for arrays and meta-types)
  Type getBase() { assert(0, "Type " ~ toString() ~ " has no base type"); }

  // Return the array dimension of this type. Eg. int[][] has two
  // dimensions, int has zero.
  int arrays() { return 0; }

  // Return the number of ints needed to store one variable of this
  // type.
  int getSize();

  // Get the scope for resolving members of this type. Examples:
  // myObject.func() -> getMemberScope called for the type of myObject,
  //                    returns the class scope, where func is resolved.
  // array.length -> getMemberScope called for ArrayType, returns a
  //                 special scope that contains the 'length' property
  // Returning null means that this type does not have any members.
  Scope getMemberScope()
    // By default we return a minimal property scope (containing
    // .sizeof etc)
    { return GenericProperties.singleton; }

  // Validate that this type actually exists. This is used to make
  // sure that all forward references are resolved.
  void validate(Floc loc) {}

  // Check if this type is equivalent to the given D type
  final bool isDType(TypeInfo ti)
    {
      char[] name = ti.toString;
      name = name[name.rfind('.')+1..$];

      switch(name)
        {
        case "int": return isInt();
        case "uint": return isUint();
        case "long": return isLong();
        case "ulong": return isUlong();
        case "float": return isFloat();
        case "double": return isDouble();
        case "bool": return isBool();
        case "dchar": return isChar();
        case "AIndex": return isArray();
        case "MIndex": return isObject();
        default:
          assert(0, "illegal type in isDType(): " ~ name);
        }
    }

  // Used by defaultInit as a shortcut for converting a variable to an
  // int array.
  static int[] makeData(T)(T val)
  {
    int data[];
    if(T.sizeof == 4) data.length = 1;
    else if(T.sizeof == 8) data.length = 2;
    else assert(0, "Unsupported type size");

    *(cast(T*)data.ptr) = val;

    return data;
  }

  // Get the default initializer for this type. The assembler deals
  // with data in terms of ints (4 byte chunks), so we return the data
  // as an int[]. The default initializer should be an illegal value
  // when possible (null pointers, nan, etc) to catch mistakenly
  // uninitialized variables as quickly as possible. This will usually
  // be the same init value as in D.
  int[] defaultInit();

  // Generate assembler code that pushes the default initializer on
  // the stack. TODO: This should become a general function in the
  // assembler to push any int[]. Pass it a type so it can check the
  // size automatically as well.
  final void pushInit()
    {
      int[] def = defaultInit();
      assert(def.length == getSize, "default initializer is not the correct size");
      setLine();

      tasm.pushArray(def);
    }

  // Compare types
  final int opEquals(Type t)
    {
      if(toString != t.toString) return 0;

      // TODO: In our current system, if the name and array dimension
      // match, we must be the same type. In the future we might have
      // to add more tests here however. For example, two structs of
      // the same name might be defined in different classes. The best
      // solution is perhaps to always make sure the type name is
      // unique.

      return 1;
    }

  final char[] toString() { return name; }

  // Cast the expression orig to this type. Uses canCastTo to
  // determine if a cast is possible. The 'to' parameter is used in
  // error messages to describe the destination.
  final void typeCast(ref Expression orig, char[] to)
    {
      if(orig.type == this) return;

      // Replace the expression with a CastExpression. This acts as a
      // wrapper that puts in the conversion code after expression is
      // evaluated.
      if(orig.type.canCastTo(this))
        orig = new CastExpression(orig, this);
      else
        fail(format("Cannot implicitly cast %s of type %s to %s of type %s",
                    orig.toString, orig.typeString,
                    to, this), orig.loc);
    }

  // Do explicit type casting. This allows for a wider range of type
  // conversions.
  final void typeCastExplicit(ref Expression orig)
    {
      if(orig.type == this) return;

      if(orig.type.canCastToExplicit(this) || orig.type.canCastTo(this))
        orig = new CastExpression(orig, this);
      else
        fail(format("Cannot cast %s of type %s to type %s",
                    orig.toString, orig.typeString,
                    this), orig.loc);
    }

  // Do compile-time type casting. Gets orig.evalCTime() and returns
  // the converted result.
  final int[] typeCastCTime(Expression orig, char[] to)
    {
      int[] res = orig.evalCTime();

      assert(res.length == orig.type.getSize);

      if(orig.type.canCastCTime(this))
        res = orig.type.doCastCTime(res, this, orig.loc);
      else
        fail(format("Cannot cast %s of type %s to %s of type %s (at compile time)",
                    orig.toString, orig.typeString,
                    to, this), orig.loc);

      assert(res.length == getSize);

      return res;
    }

  // Can this type be cast to the parameter type? This function is not
  // required to handle cases where the types are the same.
  bool canCastTo(Type to)
    {
      return false; // By default we can't cast anything
    }

  // Can the type be explicitly cast? This function does not have to
  // handle cases where canCastTo is true. You only need to use it in
  // cases where explicit casting is allowed but implicit is not, such
  // as casting from float to int.
  bool canCastToExplicit(Type to)
    {
      return false;
    }

  // Returns true if the cast can be performed at compile time. This
  // is usually true if we can cast at runtime.
  bool canCastCTime(Type to)
    { return canCastTo(to) || canCastToExplicit(to); }

  // Returns true if the types are equal or if canCastTo returns true.
  final bool canCastOrEqual(Type to)
    {
      return to == this || canCastTo(to);
    }

  // Do the cast in the assembler. Must handle both implicit and
  // explicit casts.
  void evalCastTo(Type to, Floc lc)
    {
      assert(0, "evalCastTo not implemented for type " ~ toString);
    }

  // Do the cast in the compiler. Must handle both implicit and
  // explicit casts.
  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      assert(0, "doCastCTime not implemented for type " ~ toString);
    }

  // Cast variable of this type to string
  char[] valToString(int[] data)
    {
      assert(0, "valToString not implemented for " ~ toString);
    }

  final AIndex valToStringIndex(int[] data)
    {
      assert(data.length == getSize);
      return monster.vm.arrays.arrays.create(valToString(data)).getIndex;
    }

  void parse(ref TokenArray toks) {assert(0, name);}
  void resolve(Scope sc) {assert(0, name);}

  /* Cast two expressions to their common type, if any. Fail if not
     possible. Examples of possible outcomes:
  
     int, int           -> does nothing
     float, int         -> converts the second paramter to float
     int, float         -> converts the first to float
     bool, int          -> throws an error

     For various integer types, special rules apply. These are (for
     the time being):

     ulong, any         -> ulong
     long, any 32bit    -> long
     int, uint          -> uint

     Similar types (eg. uint, uint) will never convert types.
   */
  static void castCommon(ref Expression e1, ref Expression e2)
    {
      Type t1 = e1.type;
      Type t2 = e2.type;

      if(t1 == t2) return;

      Type common;

      // Apply integral promotion rules first. TODO: Apply polysemous
      // rules later.
      if(t1.isIntegral && t2.isIntegral)
        {
          // ulong dominates all other types
          if(t1.isUlong) common = t1;
          else if(t2.isUlong) common = t2;

          // long dominates over 32-bit values
          else if(t1.isLong) common = t1;
          else if(t2.isLong) common = t2;

          // unsigned dominates over signed
          else if(t1.isUint) common = t1;
          else if(t2.isUint) common = t2;
          else
            {
              assert(t1.isInt && t2.isInt, "unknown integral type encountered");
              assert(0, "should never get here");
            }
        }
      else
        {
          // Find the common type
          if(t1.canCastTo(t2)) common = t2; else
          if(t2.canCastTo(t1)) common = t1; else
            fail(format("Cannot implicitly cast %s of type %s to %s of type %s, or vice versa.", e1, t1, e2, t2), e1.loc);
        }

      // Wrap the expressions in CastExpression blocks if necessary.
      common.typeCast(e1, "");
      common.typeCast(e2, "");
    }
}

// Internal types are types that are used internally by the compiler,
// eg. for type conversion or for special syntax. They can not be used
// for variables or values, neither directly nor indirectly.
abstract class InternalType : Type
{
 override:
  Scope getMemberScope() { return null; }
 final:
  bool isLegal() { return false; }
  int[] defaultInit() {assert(0, name);}
  int getSize() { return 0; }
}

// Handles package names, in expressions like Package.ClassName. It is
// only used for these expressions and never for anything else.
class PackageType : InternalType
{
  PackageScope sc;

  this(PackageScope _sc)
    {
      sc = _sc;
      name = sc.toString() ~ " (package)";
    }

 override:
  Scope getMemberScope()
    {
      assert(sc !is null);
      return sc;
    }

  bool isPackage() { return true; }
}

// Handles the 'null' literal. This type is only used for
// conversions. You cannot declare variables of the nulltype.
class NullType : InternalType
{
  this() { name = "null-type"; }

  bool canCastTo(Type to)
    {
      return to.isArray || to.isObject || to.isEnum || to.isFuncRef;
    }

  void evalCastTo(Type to, Floc lc)
    {
      // Always evaluable at compile time - there is no runtime value
      // to convert.
      tasm.pushArray(doCastCTime(null, to, lc));
    }

  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      // For all the types we currently support, 'null' is always
      // equal to the type's default initializer.
      assert(data.length == 0);
      return to.defaultInit();
    }
}

// Helper template for BasicType
abstract class TypeHolderBase
{
  char[] getString(int[] data);
}
class TypeHolder(T) : TypeHolderBase
{
  T toValue(int[] data)
    {
      static if(!is(T==bool))
        assert(data.length*4 == T.sizeof);
      return *(cast(T*)data.ptr);
    }

  char[] getString(int [] data)
    {
      static if(is(T == dchar))
        return toUTF8([toValue(data)]);
      else
        return .toString(toValue(data));
    }
}

// Handles all the built-in types. These are: int, uint, long, ulong,
// float, double, bool, char and the "void" type, which is represented
// by an empty string. The void type is only allowed in special cases
// (currently only in function return types), and is not parsed
// normally through identify()/parse(). Instead it is created directly
// with the constructor.
class BasicType : Type
{
  this() {}

  // Create the given type directly without parsing. Use an empty
  // string "" for the void type. This is the only way a void type can
  // be created.
  this(char[] tn)
    {
      name = tn;
      if(name == "") name = "(none)";

      if(!isBasic(name))
        fail("BasicType does not support type " ~ tn);

      // Cache the class to save some overhead
      store[tn] = this;
    }

  TypeHolderBase tph;
  TypeHolderBase getHolder()
    {
      if(tph is null)
        {
          if(isInt) tph = new TypeHolder!(int);
          if(isUint) tph = new TypeHolder!(uint);
          if(isLong) tph = new TypeHolder!(long);
          if(isUlong) tph = new TypeHolder!(ulong);
          if(isFloat) tph = new TypeHolder!(float);
          if(isDouble) tph = new TypeHolder!(double);
          if(isChar) tph = new TypeHolder!(dchar);
          if(isBool) tph = new TypeHolder!(bool);
        }
      assert(!isVoid);

      return tph;
    }

  private static BasicType[char[]] store;

  // Get a basic type of the given name. This will not allocate a new
  // instance if another instance already exists.
  static BasicType get(char[] tn)
    {
      if(tn in store) return store[tn];

      // Automatically adds itself to the store
      return new BasicType(tn);
    }

  // Shortcuts
  static BasicType getVoid() { return get(""); }
  static BasicType getInt() { return get("int"); }
  static BasicType getUint() { return get("uint"); }
  static BasicType getLong() { return get("long"); }
  static BasicType getUlong() { return get("ulong"); }
  static BasicType getFloat() { return get("float"); }
  static BasicType getDouble() { return get("double"); }
  static BasicType getChar() { return get("char"); }
  static BasicType getBool() { return get("bool"); }

  static bool isBasic(char[] tn)
    {
      return (tn == "(none)" || tn == "int" || tn == "float" || tn == "char" ||
              tn == "bool" || tn == "uint" || tn == "long" ||
              tn == "ulong" || tn == "double");
    }

  static bool canParse(TokenArray toks)
    {
      Token t;
      if(!isNext(toks, TT.Identifier, t)) return false;
      return isBasic(t.str);
    }

 override:
  void parse(ref TokenArray toks)
    {
      Token t;

      reqNext(toks, TT.Identifier, t);
      assert(isBasic(t.str));

      // Get the name and the line from the token
      name = t.str;
      loc = t.loc;

      if(name == "") name = "(none)";
    }

  bool isInt() { return name == "int"; }
  bool isUint() { return name == "uint"; }
  bool isLong() { return name == "long"; }
  bool isUlong() { return name == "ulong"; }
  bool isChar() { return name == "char"; }
  bool isBool() { return name == "bool"; }
  bool isFloat() { return name == "float"; }
  bool isDouble() { return name == "double"; }
  bool isVoid() { return name == "(none)"; }

  Scope getMemberScope()
    {
      if(isInt) return IntProperties.singleton;
      if(isUint) return UintProperties.singleton;
      if(isLong) return LongProperties.singleton;
      if(isUlong) return UlongProperties.singleton;
      if(isFloat) return FloatProperties.singleton;
      if(isDouble) return DoubleProperties.singleton;

      if(isChar || isBool)
        return GenericProperties.singleton;

      assert(isVoid);
      return null;
    }

  // List the implisit conversions that are possible
  bool canCastTo(Type to)
    {
      // If options.d allow it, we can implicitly convert back from
      // floats to integral types.
      static if(implicitTruncate)
        {
          if(isFloating && to.isIntegral)
            return true;
        }

      // We can convert between all integral types
      if(to.isIntegral) return isIntegral;

      // All numerical types can be converted to floating point
      if(to.isFloating) return isNumerical;

      // These types can be converted to strings.
      if(to.isString)
        return isNumerical || isChar || isBool;

      return false;
    }

  bool canCastToExplicit(Type to)
    {
      if(isFloating && to.isIntegral)
        return true;

      return false;
    }

  void evalCastTo(Type to, Floc lc)
    {
      assert(this != to);
      assert(!isVoid);

      int fromSize = getSize();
      int toSize = to.getSize();
      bool fromSign = isInt || isLong || isFloat || isBool;

      if(isFloating && to.isIntegral)
        tasm.castFloatToInt(this, to);
      else if(to.isInt || to.isUint)
        {
          assert(isIntegral);
          if(isLong || isUlong)
            tasm.castLongToInt();
        }
      else if(to.isLong || to.isUlong)
        {
          if(isInt || isUint) tasm.castIntToLong(fromSign);
          else assert(isUlong || isLong);
        }
      else if(to.isFloat || to.isDouble)
        {
          if(isIntegral)
            tasm.castIntToFloat(this, to);
          else if(isFloating)
            tasm.castFloatToFloat(fromSize, toSize);
          else assert(0);
        }
      else if(to.isString)
        {
          assert(!isVoid);

          // Create an array from one element on the stack
          if(isChar) tasm.popToArray(1, 1);
          else
            tasm.castToString(this);
        }
      else
        fail("Conversion " ~ toString ~ " to " ~ to.toString ~
             " not implemented.", lc);
    }

  char[] valToString(int[] data)
    {
      assert(data.length == getSize);
      assert(!isVoid);

      return getHolder().getString(data);
    }

  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      assert(this != to);
      assert(!isVoid);
      assert(!to.isVoid);

      if(to.isString)
        return [valToStringIndex(data)];

      int fromSize = getSize();
      int toSize = to.getSize();

      assert(data.length == fromSize);
      bool fromSign = isInt || isLong || isFloat || isBool;

      // Set up a new array to hold the result
      int[] toData = new int[toSize];

      if(isFloating && to.isIntegral)
        {
          int *iptr = cast(int*)toData.ptr;
          uint *uptr = cast(uint*)toData.ptr;
          long *lptr = cast(long*)toData.ptr;
          ulong *ulptr = cast(ulong*)toData.ptr;

          if(isFloat)
            {
              float f = *(cast(float*)data.ptr);
              if(to.isInt) *iptr = cast(int) f;
              else if(to.isUint) *uptr = cast(uint) f;
              else if(to.isLong) *lptr = cast(long) f;
              else if(to.isUlong) *ulptr = cast(ulong) f;
              else assert(0);
            }
          else if(isDouble)
            {
              double f = *(cast(double*)data.ptr);
              if(to.isInt) *iptr = cast(int) f;
              else if(to.isUint) *uptr = cast(uint) f;
              else if(to.isLong) *lptr = cast(long) f;
              else if(to.isUlong) *ulptr = cast(ulong) f;
              else assert(0);
            }
          else assert(0);
        }
      else if(to.isInt || to.isUint)
        {
          assert(isIntegral);
          // Just pick out the least significant int
          toData[] = data[0..1];
        }
      else if(to.isLong || to.isUlong)
        {
          if(isInt || isUint)
            {
              toData[0] = data[0];
              if(fromSign && to.isLong && data[0] < 0) toData[1] = -1;
              else toData[1] = 0;
            }
          else assert(isUlong || isLong);
        }
      else if(to.isFloat)
        {
          assert(isNumerical);

          float *fptr = cast(float*)toData.ptr;

          if(isInt) *fptr = data[0];
          else if(isUint) *fptr = cast(uint)data[0];
          else if(isLong) *fptr = *(cast(long*)data.ptr);
          else if(isUlong) *fptr = *(cast(ulong*)data.ptr);
          else if(isDouble) *fptr = *(cast(double*)data.ptr);
          else assert(0);
        }
      else if(to.isDouble)
        {
          assert(isNumerical);

          assert(toData.length == 2);
          double *fptr = cast(double*)toData.ptr;

          if(isInt) *fptr = data[0];
          else if(isUint) *fptr = cast(uint)data[0];
          else if(isLong) *fptr = *(cast(long*)data.ptr);
          else if(isUlong) *fptr = *(cast(ulong*)data.ptr);
          else if(isFloat) *fptr = *(cast(float*)data.ptr);
          else assert(0);
        }
      else
        fail("Compile time conversion " ~ toString ~ " to " ~ to.toString ~
             " not implemented.", lc);

      assert(toData.length == toSize);
      assert(toData.ptr !is data.ptr);
      return toData;
    }

  int getSize()
    {
      if( isInt || isUint || isFloat || isChar || isBool )
        return 1;

      if( isLong || isUlong || isDouble )
        return 2;

      if( isVoid )
        return 0;

      assert(0, "getSize() does not handle type '" ~ name ~ "'");
    }

  void resolve(Scope sc)
    {
      // Check that our given name is indeed valid
      assert(isBasic(name));
    }

  int[] defaultInit()
  {
    int data[];

    // Ints default to 0, bools to false
    if(isInt || isUint || isBool) data = makeData!(int)(0);
    // Ditto for double-size ints
    else if(isLong || isUlong) data = makeData!(long)(0);
    // Chars default to an illegal utf-32 value
    else if(isChar) data = makeData!(int)(0x0000FFFF);
    // Floats default to NaN
    else if(isFloat) data = makeData!(float)(float.nan);
    else if(isDouble) data = makeData!(double)(double.nan);
    else
      assert(0, "Type '" ~ name ~ "' has no default initializer");

    assert(data.length == getSize, "size mismatch in defaultInit");

    return data;
  }
}

// Represents a normal class name.
class ObjectType : Type
{
 final:
 private:
  // Class that we represent (set by resolve()). Variables of this
  // type may point to objects of this class, or to objects of
  // subclasses. We only use an index, since classes might be forward
  // referenced.
  CIndex clsIndex;

 public:

  this(Token t)
    {
      // Get the name and the line from the token
      name = t.str;
      loc = t.loc;
    }

  this(MonsterClass mc)
    {
      assert(mc !is null);
      name = mc.name.str;
      loc = mc.name.loc;
      clsIndex = mc.gIndex;
    }

  MonsterClass getClass(Floc loc = Floc.init)
    {
      assert(clsIndex != 0);

      if(!global.isLoaded(clsIndex))
        fail("Cannot use " ~ name ~
             ": class not found or forward reference", loc);

      return global.getClass(clsIndex);
    }

 override:
  void validate(Floc loc)
    {
      // getClass does most of the error checking we need
      auto mc = getClass(loc);

      // check that we're not using a module as a type
      if(mc.isModule)
        fail("Cannot use module " ~ name ~ " as a type", loc);
    }

  char[] valToString(int[] data)
    {
      // Use the object's toString function
      assert(data.length == 1);
      if(data[0] == 0) return "(null object)";
      auto mo = getMObject(cast(MIndex)data[0]);
      return mo.toString();
    }

  int getSize() { return 1; }
  bool isObject() { return true; }
  int[] defaultInit() { return makeData!(int)(0); }

  bool canCastCTime(Type to) { return false; }

  // Used internally below, to get the class from another object type.
  private MonsterClass getOther(Type other)
    {
      assert(other !is this);
      assert(other.isObject);
      auto ot = cast(ObjectType)other;
      assert(ot !is null);
      auto cls = ot.getClass();
      assert(cls !is null);
      assert(cls !is getClass());
      return cls;
    }

  bool canCastTo(Type type)
    {
      assert(clsIndex != 0);

      if(type.isString) return true;

      if(type.isObject)
        {
          MonsterClass us = getClass();
          MonsterClass other = getOther(type);

          // Allow implicit downcasting if options.d says so.
          static if(implicitDowncast)
            {
              if(other.childOf(us))
                return true;
            }

          // We can always upcast implicitly
          return other.parentOf(us);
        }

      return false;
    }

  bool canCastToExplicit(Type type)
    {
      // We can explicitly cast to a child class, and let the VM
      // handle any errors.
      if(type.isObject)
        {
          auto us = getClass();
          auto other = getOther(type);

          return other.childOf(us);
        }
    }

  void evalCastTo(Type to, Floc lc)
    {
      assert(clsIndex != 0);
      assert(canCastTo(to) || canCastToExplicit(to));

      if(to.isObject)
        {
          auto us = getClass();
          auto other = getOther(to);

          // Upcasting doesn't require any action
          if(other.parentOf(us)) {}

          // Downcasting (from parent to child) requires that VM
          // checks the runtime type of the object.
          else if(other.childOf(us))
            // Use the global class index
            tasm.downCast(other.getIndex());

          // We should never get here
          else assert(0);

          return;
        }

      assert(to.isString);
      tasm.castToString(this);
    }

  // Members of objects are resolved in the class scope.
  Scope getMemberScope()
    {
      assert(getClass !is null);
      assert(getClass.sc !is null);
      return getClass().sc;
    }

  // This is called when the type is defined, and it can forward
  // reference classes that do not exist yet. The classes must exist
  // by the time getClass() is called though, usually when the class
  // body (not just the header) is being resolved.
  void resolve(Scope sc)
    {
      // Insert the class into the global scope as a forward
      // reference. Note that this means the class may not be part of
      // any other package than 'global' when it is loaded later.
      if(clsIndex == 0)
        clsIndex = global.getForwardIndex(name);

      assert(clsIndex != 0);
    }
}

class ArrayType : Type
{
  // What type is this an array of?
  Type base;

  static ArrayType str;

  static ArrayType getString()
    {
      if(str is null)
        str = new ArrayType(BasicType.getChar);
      return str;
    }

  static ArrayType get(Type base)
    {
      // TODO: We can cache stuff here later
      return new ArrayType(base);
    }

  this(Type btype)
    {
      base = btype;
      name = base.name ~ "[]";
      loc = base.loc;
    }

  ArrayRef *getArray(int[] data)
    {
      assert(data.length == 1);
      return monster.vm.arrays.arrays.getRef(cast(AIndex)data[0]);
    }

  override:
  void validate(Floc loc) { assert(base !is null); base.validate(loc); }
  int arrays() { return base.arrays() + 1; }
  int getSize() { return 1; }
  int[] defaultInit() { return makeData!(int)(0); }
  Type getBase() { return base; }

  Scope getMemberScope()
    {
      return ArrayProperties.singleton;
    }

  // All arrays can be cast to string
  bool canCastTo(Type to)
    {
      // Strings can be cast to char, but the conversion is only valid
      // if the string is one char long.
      if(isString) return to.isChar;

      // Conversion to string is always allowed, and overrides other
      // array conversions. TODO: This will change later when we get
      // the generic 'var' type. Then implicit casting to string will
      // be unnecessary.
      if(to.isString)
        return true;

      if(to.isArray)
        return getBase().canCastTo(to.getBase());

      return false;
    }

  void evalCastTo(Type to, Floc lc)
    {
      if(isString)
        {
          assert(to.isChar);
          tasm.castStrToChar();
          return;
        }

      if(to.isString)
        {
          tasm.castToString(this);
          return;
        }

      if(to.isArray && getBase().canCastTo(to.getBase()))
        fail("Casting arrays at runtime not implemented yet", lc);

      assert(0);
    }

  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      if(isString)
        {
          assert(to.isChar);

          // When casting from string to char, we pick out the first
          // (and only) character from the string.

          auto arf = getArray(data);

          // FIXME: These error messages don't produce a file/line
          // number. Using our own loc doesn't work. I think we need
          // to pass it on to doCastCTime.

          if(arf.carr.length == 0)
            fail("Cannot cast empty string to 'char'", lc);

          assert(arf.elemSize == 1);

          if(arf.carr.length > 1)
            fail("Cannot cast string " ~ valToString(data) ~
                 " to 'char': too long", lc);

          return [arf.iarr[0]];
        }

      if(to.isArray && getBase().canCastTo(to.getBase()))
        {
          auto arf = getArray(data);
          assert(arf.iarr.length > 0, "shouldn't need to cast an empty array");

          // The types
          Type oldt = getBase();
          Type newt = to.getBase();

          assert(oldt.canCastCTime(newt),
                 "oops, cannot cast at compile time");

          // Set up the source and destination array data
          int len = arf.length();
          int olds = oldt.getSize();
          int news = newt.getSize();

          int[] src = arf.iarr;
          int[] dst = new int[len * news];

          assert(src.length == len * olds);

          for(int i=0; i<len; i++)
            {
              int[] slice = src[i*olds..(i+1)*olds];
              slice = oldt.doCastCTime(slice, newt, lc);
              assert(slice.length == news);
              dst[i*news..(i+1)*news] = slice;
            }

          // Create the array index
          bool wasConst = arf.isConst();
          arf = monster.vm.arrays.arrays.create(dst, news);
          assert(arf.length() == len);

          // Copy the const setting from the original array
          if(wasConst)
            arf.flags.set(AFlags.Const);

          // Return the index
          return [arf.getIndex()];
        }

      if(to.isString)
        return [valToStringIndex(data)];

      assert(0);
    }

  char[] valToString(int[] data)
    {
      // Get the array reference
      ArrayRef *arf = getArray(data);

      // Empty array?
      if(arf.iarr.length == 0) return "[]";

      assert(arf.elemSize == base.getSize);
      assert(arf.iarr.length == arf.length * arf.elemSize);

      if(isString) return format("\"%s\"", arf.carr);

      char[] res = "[";

      // For element of the array, run valToString on the appropriate
      // data
      for(int i; i<arf.iarr.length; i+=arf.elemSize)
        {
          if(i != 0) res ~= ',';
          res ~= base.valToString(arf.iarr[i..i+arf.elemSize]);
        }
      return res ~ "]";
    }

  // We are a string (char[]) if the base type is char.
  bool isString()
    {
      return base.isChar();
    }

  void resolve(Scope sc)
    {
      base.resolve(sc);

      if(base.isReplacer())
        base = base.getBase();
      name = base.name ~ "[]";
    }
}

// Type used for function references. Variables of this type consists
// of a object index followed by a global function index.
class FuncRefType : Type
{
  // Function signature information
  Type retType;
  Type params[];
  bool isConst[];
  bool isVararg;

  // For parsed types
  this() {}

  this(IntFuncType f)
    {
      auto fn = f.func;

      retType = fn.type;
      isVararg = fn.isVararg();
      params.length = fn.params.length;
      isConst.length = params.length;

      foreach(i, v; fn.params)
        {
          params[i] = v.type;
          isConst[i] = v.isConst;
        }

      createName();
    }

  void createName()
    {
      name = "function";
      if(!retType.isVoid)
        name ~= " " ~ retType.toString();
      name ~= "(";

      foreach(i, v; params)
        {
          if(i > 0)
            name ~= ",";
          name ~= v.toString();

          if(isVararg && i == params.length-1)
            name ~= "...";
        }

      name ~= ")";
    }

  static bool canParseRem(ref TokenArray toks)
    {
      if(!isNext(toks, TT.Function))
        return false;

      // Return type?
      if(toks.length && toks[0].type != TT.LeftParen)
        Type.canParseRem(toks);

      skipParens(toks);
      return true;
    }

  // Same as the above but it doesn't change the array (no 'ref'
  // parameter)
  static bool canParse(TokenArray toks)
    { return canParseRem(toks); }

  // Calculate the stack imprint
  int getImprint()
    {
      assert(retType !is null);
      int res = retType.getSize();

      foreach(p; params)
        res -= p.getSize();

      return res;
    }

 override:
  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Function);

      if(!isNext(toks, TT.LeftParen))
        {
          retType = Type.identify(toks);
          reqNext(toks, TT.LeftParen);
        }
      else
        retType = BasicType.getVoid();

      if(!isNext(toks, TT.RightParen))
        {
          do
            {
              params ~= Type.identify(toks);
            }
          while(isNext(toks, TT.Comma));
          reqNext(toks, TT.RightParen);
        }
    }

  void resolve(Scope sc)
    {
      retType.resolve(sc);
      foreach(p; params)
        p.resolve(sc);

      createName();
    }

  void validate(Floc f)
    {
      retType.validate(f);
      foreach(p; params)
        p.validate(f);
    }

  int getSize() { return 2; }
  bool isFuncRef() { return true; }
  int[] defaultInit() { return [0, -1]; }
  Scope getMemberScope() { return FuncRefProperties.singleton; }
}

// Type used for internal references to functions. This is NOT the
// same as the type used for function references ("pointers") - see
// FuncRefType for that.
class IntFuncType : InternalType
{
  Function *func;
  bool isMember;

  this(Function *fn, bool isMemb)
    {
      func = fn;
      assert(fn !is null);
      isMember = isMemb;

      name="Function";
    }

 override:
  bool isIntFunc() { return true; }
}

class EnumType : Type
{
  // Enum entries
  EnumEntry[] entries;
  EnumScope sc;
  char[] initString = " (not set)";

  // Lookup tables
  EnumEntry* nameAA[char[]];
  EnumEntry* valueAA[long];

  // Fields
  FieldDef fields[];

  long
    minVal = long.max,
    maxVal = long.min;

  Token nameTok;

  EnumEntry *lookup(long val)
    {
      auto p = val in valueAA;
      if(p is null)
        return null;
      return *p;
    }

  EnumEntry *lookup(char[] str)
    {
      auto p = str in nameAA;
      if(p is null) return null;
      return *p;
    }

  int findField(char[] str)
    {
      foreach(i, fd; fields)
        if(fd.name.str == str)
          return i;
      return -1;
    }

 override:

  bool isEnum() { return true; }

  int[] defaultInit() { return [0]; }
  int getSize() { return 1; }

  void resolve(Scope last)
  {
    assert(sc is null, "resolve() called more than once");

    initString = name ~ initString;

    foreach(i, ref ent; entries)
      {
        // Make sure there are no naming conflicts.
        last.clearId(ent.name);

        // Assign an internal value to each entry
        ent.index = i+1;

        // Set the printed value to be "Enum.Name"
        ent.stringValue = name ~ "." ~ ent.name.str;

        // Create an AA for values, and one for the names. This is also
        // where we check for duplicates in both.
        if(ent.name.str in nameAA)
          fail("Duplicate entry '" ~ ent.name.str ~ "' in enum", ent.name.loc);
        if(ent.value in valueAA)
          fail("Duplicate value " ~ .toString(ent.value) ~ " in enum", ent.name.loc);
        nameAA[ent.name.str] = &ent;
        valueAA[ent.value] = &ent;

        if(ent.value > maxVal) maxVal = ent.value;
        if(ent.value < minVal) minVal = ent.value;
      }

    // Create the scope
    sc = new EnumScope(this);

    // Check the fields
    foreach(ref fd; fields)
      {
        last.clearId(fd.name);
        if(fd.name.str in nameAA)
          fail("Field name cannot match value name " ~ fd.name.str, fd.name.loc);

        fd.type.resolve(last);
        if(fd.type.isReplacer)
          fd.type = fd.type.getBase();

        fd.type.validate(fd.name.loc);
      }

    // Resolve and check field expressions.
    foreach(ref ent; entries)
      {
        // Check number of expressions
        if(ent.exp.length > fields.length)
          fail(format("Too many fields in enum line (expected %s, found %s)",
                      fields.length, ent.exp.length),
               ent.name.loc);

        ent.fields.length = fields.length;

        foreach(i, ref fe; ent.exp)
          {
            assert(fe !is null);
            fe.resolve(last);

            // Check the types
            fields[i].type.typeCast(fe, format("field %s (%s)",
                                               i+1, fields[i].name.str));

            // And that they are all compile time expressions
            if(!fe.isCTime)
              fail("Cannot evaluate " ~ fe.toString ~ " at compile time", fe.loc);
          }

        // Finally, get the values
        foreach(i, ref int[] data; ent.fields)
          {
            if(i < ent.exp.length)
              data = ent.exp[i].evalCTime();
            else
              // Use the init value if no field is value is given
              data = fields[i].type.defaultInit();

            assert(data.length == fields[i].type.getSize);
          }

        // Clear the expression array since we don't need it anymore
        ent.exp = null;
      }
  }

  bool canCastTo(Type to)
    {
      // Can always cast to string
      if(to.isString) return true;

      // The value is a long, so we can always cast to types that long
      // can be cast to.
      if(BasicType.getLong().canCastOrEqual(to)) return true;

      // Check each field from left to right. If the field can be cast
      // to the given type, then it's ok.
      foreach(f; fields)
        if(f.type.canCastOrEqual(to))
          return true;

      return false;
    }

  void evalCastTo(Type to, Floc lc)
    {
      // Convert the enum name to a string
      if(to.isString)
        {
          tasm.castToString(this);
          return;
        }

      auto lng = BasicType.getLong();
      if(lng.canCastOrEqual(to))
        {
          // Get the value
          tasm.getEnumValue(this);
          // Cast it if necessary
          if(to != lng)
            lng.evalCastTo(to, lc);
          return;
        }

      // Check the fields
      foreach(i, f; fields)
        if(f.type.canCastOrEqual(to))
          {
            // Get the field value from the enum
            tasm.getEnumValue(this, i);

            // If the type doesn't match exactly, convert it.
            if(f.type != to)
              f.type.evalCastTo(to, lc);

            return;
          }

      assert(0);
    }

  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      if(to.isString)
        return [valToStringIndex(data)];

      // This code won't run yet, because the enum fields are
      // properties and we haven't implemented ctime property reading
      // yet. Leave this assert in here so that we remember to test it
      // later. TODO.
      assert(0, "finished, but not tested");

      // Get the enum index
      assert(data.length == 1);
      int v = data[0];

      // Check that we were not given a zero index
      if(v-- == 0)
        fail("Cannot get value of fields from an empty Enum variable.", lc);

      // Get the entry
      assert(v >= 0 && v < entries.length);
      auto ent = &entries[v];

      auto lng = BasicType.getLong();
      if(lng.canCastOrEqual(to))
        {
          // Get the value
          int[] val = (cast(int*)&ent.value)[0..2];
          // Cast it if necessary
          if(to != lng)
            val = lng.doCastCTime(val, to, lc);

          return val;
        }

      // Check the fields
      foreach(i, f; fields)
        if(f.type.canCastOrEqual(to))
          {
            // Get the field value from the enum
            int[] val = ent.fields[i];

            // If the type doesn't match exactly, convert it.
            if(f.type != to)
              val = f.type.doCastCTime(val, to, lc);

            return val;
          }

      assert(0);
    }

  // TODO: In this case, we could override valToStringIndex as well,
  // and return a cached, constant string index to further optimize
  // memory usage.
  char[] valToString(int[] data)
    {
      assert(data.length == 1);
      int v = data[0];
      assert(v >= 0 && v <= entries.length);

      // v == 0 means that no value is set - return a default string
      // value
      if(v == 0)
        return initString;

      else return entries[v-1].stringValue;
    }

  Scope getMemberScope()
    { return sc; }
}

class StructType : Type
{
 private:
  StructDeclaration sd;

  void setup()
    {
      if(!set)
        sd.resolve(sc.getParent);
      else
        {
          // We might get here if this function is being called
          // recursively from sd.resolve. This only happens when the
          // struct contains itself somehow. In that case, size will
          // not have been set yet.
          if(size == -1)
            fail("Struct " ~ name ~ " indirectly contains itself", loc);
        }

      assert(set);
      assert(size != -1);
    }

 public:

  int size = -1;
  StructScope sc; // Scope of the struct interior
  int[] defInit;
  bool set; // Have vars and defInit been set?

  // Member variables
  Variable*[] vars;

  this(StructDeclaration sdp)
    {
      sd = sdp;
      name = sd.name.str;
      loc = sd.name.loc;
    }

 override:
  // Calls validate for all member types
  void validate(Floc loc)
    {
      foreach(v; vars)
        v.type.validate(loc);
    }

  // Resolves member
  void resolve(Scope sc) {}

  bool isStruct() { return true; }

  int getSize()
    {
      setup();
      assert(size != -1);
      return size;
    }

  int[] defaultInit()
    {
      setup();
      assert(defInit.length == getSize);
      return defInit;
    }

  // Scope getMemberScope() { return sc; }
}

// A 'delayed lookup' type - can replace itself after resolve is
// called.
abstract class ReplacerType : InternalType
{
 protected:
  Type realType;

 public:
 override:
  Type getBase() { assert(realType !is null); return realType; }
  bool isReplacer() { return true; }
}

// Type names that consist of an identifier - classes, structs, enums,
// etc. We can't know what type it is until we resolve it.
class UserType : ReplacerType
{
 private:
  Token ids[];

 public:

  static bool canParse(TokenArray toks)
    {
      if(!isNext(toks, TT.Identifier)) return false;
      return true;
    }

  void parse(ref TokenArray toks)
    {
      Token id;
      reqNext(toks, TT.Identifier, id);

      // Get the name and the line from the first token
      name = id.str~"(replacer)";
      loc = id.loc;

      ids ~= id;

      // Parse any following identifiers, separated by dots.
      while(isNext(toks,TT.Dot))
        {
          reqNext(toks, TT.Identifier, id);
          ids ~= id;
        }
    }

  void resolve(Scope sc)
    {
      assert(ids.length >= 1);

      // The top-most identifier is looked up in imported scopes
      auto first = ids[0];
      auto sl = sc.lookupImport(first);
      if(sl.isImport)
        sl = sl.imphold.lookup(first);

      // The scope in which to resolve the class lookup.
      Scope lastScope = sc;

      // Loop through the list and look up each member.
      foreach(int ind, idt; ids)
        {
          if(ind == 0) continue;

          if(sl.isClass)
            {
              // Look up the next identifier in the class scope
              assert(sl.mc !is null);
              sl.mc.requireScope();
              assert(sl.mc.sc !is null);
              sl = sl.mc.sc.lookupClass(idt);
            }
          else if(sl.isPackage)
            {
              lastScope = sl.sc;
              assert(sl.sc.isPackage);
              sl = sl.sc.lookupClass(idt);
            }
          // Was anything found at all?
          else if(!sl.isNone)
            fail(sl.name.str ~ " (which is a " ~ LTypeName[sl.ltype]
                 ~ ") does not have a type member called " ~ idt.str,
                 idt.loc);
          else
            fail("Unknown type " ~ sl.name.str, sl.name.loc);
        }

      // sl should now contain the lookup result of the last
      // identifier in the list.

      // Is it a type?
      if(sl.isType)
        // Great!
        realType = sl.type;

      // If not, maybe a class?
      else if(sl.isClass)
        {
          // Splendid
          sl.mc.requireScope();
          realType = sl.mc.objType;
          assert(realType !is null);
        }

      // Allow packages used as type names in some situations
      else if(sl.isPackage)
        {
          auto psc = cast(PackageScope)sl.sc;
          assert(psc !is null);
          realType = psc.type;
        }

      // Was anything found at all?
      else if(!sl.isNone)
        // Ouch, something was found that's not a type or class.
        fail("Cannot use " ~ sl.name.str ~ " (which is a " ~
             LTypeName[sl.ltype] ~ ") as a type!", sl.name.loc);

      if(realType is null)
        {
          // Nothing was found. Assume it's a forward reference to a
          // class. These are handled later on.
          realType = new ObjectType(sl.name);
          assert(lastScope !is null);
          realType.resolve(lastScope);
        }

      assert(realType !is this);
      assert(!realType.isReplacer);
    }
}

class TypeofType : ReplacerType
{
  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.Typeof); }

  Expression exp;

  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Typeof, loc);
      reqNext(toks, TT.LeftParen);
      exp = Expression.identify(toks);
      reqNext(toks, TT.RightParen);
    }

  void resolve(Scope sc)
    {
      // Resolve the expression in the context of the scope
      exp.resolve(sc);
      if(exp.type.isMeta)
        fail("Cannot use typeof on a meta type", exp.loc);

      realType = exp.type;
    }
}

// The 'var' type - generic type. Currently just works like 'auto' in
// D.
class GenericType : InternalType
{
  static GenericType sing;

  this() { name = "var"; }

  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.Var); }

  static GenericType getSingleton()
    {
      if(sing is null)
        sing = new GenericType;

      return sing;
    }

 override:
  bool isVar() { return true; }

  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Var, loc);
    }

  void resolve(Scope sc) {}
}

// I never meta type I didn't like!
class MetaType : InternalType
{
 protected:
  Type base;
  AIndex ai;

 public:

  static Type getBasic(char[] basic)
    {
      return BasicType.get(basic).getMeta();
    }

  this(Type b)
    {
      base = b;
      name = "(meta)"~base.toString;
      ai = 0;
    }

  bool isMeta() { return true; }

  bool canCastTo(Type type)
    {
      return type.isString;
    }

  char[] valToString(int[] data)
    {
      assert(data.length == 0);
      return base.name;
    }

  int[] cache;

  int[] doCastCTime(int[] data, Type to, Floc lc)
    {
      assert(to.isString);

      if(cache.length == 0)
        cache = [valToStringIndex(data)];

      return cache;
    }

  void evalCastTo(Type to, Floc lc)
    {
      assert(to.isString);

      // Create an array index and store it for reuse later.
      if(ai == 0)
        {
          auto arf = monster.vm.arrays.arrays.create(base.toString);
          arf.flags.set(AFlags.Const);
          ai = arf.getIndex();
        }
      assert(ai != 0);

      tasm.push(cast(uint)ai);
    }

  // Return the scope belonging to the base type. This makes int.max
  // work just like i.max. Differentiation between static and
  // non-static members is handled in the expression resolves.
  Scope getMemberScope() { return base.getMemberScope(); }
  Type getBase() { return base; }
}
