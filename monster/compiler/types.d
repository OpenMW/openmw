/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
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
import monster.vm.error;

import std.stdio;
import std.string;

/*
  List of all type classes:

  Type (abstract)
  InternalType (abstract)
  ReplacerType (abstract)
  NullType (null expression)
  BasicType (covers int, char, bool, float, void)
  ObjectType
  ArrayType
  StructType
  EnumType
  UserType (replacer for identifier-named types like structs and
            classes)
  TypeofType (replacer for typeof(x) when used as a type)
  GenericType (var)
  MetaType (type of type expressions, like writeln(int);)

 */
class TypeException : Exception
{
  Type type1, type2;

  this(Type t1, Type t2)
    {
      type1 = t1;
      type2 = t2;
      super("Unhandled TypeException on types " ~ type1.toString ~ " and " ~
            type2.toString ~ ".");
    }
}

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
      if(isNext(toks, TT.Typeof))
        {
          reqNext(toks, TT.LeftParen);
          Expression.identify(toks); // Possibly a bit wasteful...
          reqNext(toks, TT.RightParen);
          return true;
        }

      if(isNext(toks, TT.Var)) return true;

      if(!isNext(toks, TT.Identifier)) return false;

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
      // corresponding class.
      if(BasicType.canParse(toks)) t = new BasicType();
      else if(UserType.canParse(toks)) t = new UserType();
      else if(GenericType.canParse(toks)) t = new GenericType();
      else if(TypeofType.canParse(toks)) t = new TypeofType();
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

  // The complete type name including specifiers, eg. "int[]".
  char[] name;

  MetaType meta;
  final MetaType getMeta()
    {
      if(meta is null)
        meta = new MetaType(this);
      return meta;
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
  bool isStruct() { return false; }

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
  // (through automatic type inference.) This isn't currently used
  // anywhere, but we will need it later when we implement automatic
  // types.
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
  Scope getMemberScope() 
    {
      // Returning null means this type does not have any members.
      return null;
    }

  // Validate that this type actually exists. This is used to make
  // sure that all forward references are resolved.
  void validate() {}

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
  // determine if a cast is possible.
  final void typeCast(ref Expression orig)
    {
      if(orig.type == this) return;

      // Replace the expression with a CastExpression. This acts as a
      // wrapper that puts in the conversion code after expression is
      // evaluated.
      if(orig.type.canCastTo(this))
        orig = new CastExpression(orig, this);
      else
        throw new TypeException(this, orig.type);
    }

  // Do compile-time type casting. Gets orig.evalCTime() and returns
  // the converted result.
  final int[] typeCastCTime(Expression orig)
    {
      int[] res = orig.evalCTime();

      assert(res.length == orig.type.getSize);

      if(orig.type.canCastTo(this))
        res = orig.type.doCastCTime(res, this);
      else
        throw new TypeException(this, orig.type);

      assert(res.length == getSize);

      return res;
    }

  // Can this type be cast to the parameter type? This function is not
  // required to handle cases where the types are the same.
  bool canCastTo(Type to)
    {
      return false; // By default we can't cast anything
    }

  // Returns true if the cast can be performed at compile time. This
  // is usually true.
  bool canCastCTime(Type to)
    { return canCastTo(to); }

  // Returns true if the types are equal or if canCastTo returns true.
  final bool canCastOrEqual(Type to)
    {
      return to == this || canCastTo(to);
    }

  // Do the cast in the assembler. Rename to compileCastTo, perhaps.
  void evalCastTo(Type to)
    {
      assert(0, "evalCastTo not implemented for type " ~ toString);
    }

  // Do the cast in the compiler.
  int[] doCastCTime(int[] data, Type to)
    {
      assert(0, "doCastCTime not implemented for type " ~ toString);
    }

  void parse(ref TokenArray toks) {assert(0, name);}
  void resolve(Scope sc) {assert(0, name);}

  /* Cast two expressions to their common type, if any. Throw a
     TypeException exception if not possible. This exception should be
     caught elsewhere to give a more useful error message. Examples of
     possible outcomes:
  
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
          if(t2.canCastTo(t1)) common = t1;
          else throw new TypeException(t1, t2);
        }

      // Wrap the expressions in CastExpression blocks if necessary.
      common.typeCast(e1);
      common.typeCast(e2);
    }
}

// Internal types are types that are used internally by the compiler,
// eg. for type conversion or for special syntax. They can not be used
// for variables, neither directly nor indirectly.
abstract class InternalType : Type
{
 final:
  bool isLegal() { return false; }
  int[] defaultInit() {assert(0, name);}
  int getSize() {assert(0, name);}
}

// Handles the 'null' literal. This type is only used for
// conversions. You cannot declare variables of the nulltype.
class NullType : InternalType
{
  this() { name = "null-type"; }

  bool canCastTo(Type to)
    {
      return to.isArray || to.isObject;
    }

  void evalCastTo(Type to)
    {
      assert(to.getSize == 1);

      // The value of null is always zero. There is no value on the
      // stack to convert, so just push it.
      tasm.push(0);
    }

  int[] doCastCTime(int[] data, Type to)
    {
      assert(to.getSize == 1);
      assert(data.length == 0);
      return [0];
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
      if(!isBasic(tn))
        fail("BasicType does not support type " ~ tn);

      name = tn;

      // Cache the class to save some overhead
      store[tn] = this;
    }

  private static BasicType[char[]] store;

  // Get a basic type of the given name. This will not allocate a new
  // instance if another instance already exists.
  static BasicType get(char[] tn)
    {
      if(tn in store) return store[tn];

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
      return (tn == "" || tn == "int" || tn == "float" || tn == "char" ||
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
    }

  bool isInt() { return name == "int"; }
  bool isUint() { return name == "uint"; }
  bool isLong() { return name == "long"; }
  bool isUlong() { return name == "ulong"; }
  bool isChar() { return name == "char"; }
  bool isBool() { return name == "bool"; }
  bool isFloat() { return name == "float"; }
  bool isDouble() { return name == "double"; }
  bool isVoid() { return name == ""; }

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
      // We can convert between all integral types
      if(to.isIntegral) return isIntegral;

      // All numerical types can be converted to floating point
      if(to.isFloating) return isNumerical;

      // These types can be converted to strings.
      if(to.isString)
        return isNumerical || isChar || isBool;

      return false;
    }

  bool canCastCTime(Type to)
    {
      // We haven't implemented compile time string casting yet
      if(to.isString) return false;
      return canCastTo(to);
    }

  void evalCastTo(Type to)
    {
      assert(this != to);
      assert(!isVoid);

      int fromSize = getSize();
      int toSize = to.getSize();
      bool fromSign = isInt || isLong || isFloat || isBool;

      if(to.isInt || to.isUint)
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

          if(isIntegral)
            tasm.castIntToString(this);
          else if(isFloating)
            tasm.castFloatToString(this);
          else if(isBool) tasm.castBoolToString();

          // Create an array from one element on the stack
          else if(isChar) tasm.popToArray(1, 1);
          else assert(0, name ~ " not done yet");
        }
      else
        fail("Conversion " ~ toString ~ " to " ~ to.toString ~
             " not implemented.");
    }

  int[] doCastCTime(int[] data, Type to)
    {
      assert(this != to);
      assert(!isVoid);

      int fromSize = getSize();
      int toSize = to.getSize();
      bool fromSign = isInt || isLong || isFloat || isBool;

      assert(data.length == fromSize);

      if(to.isInt || to.isUint)
        {
          assert(isIntegral);
          data = data[0..1];
        }
      else if(to.isLong || to.isUlong)
        {
          if(isInt || isUint)
            {
              if(fromSign && to.isLong && data[0] < 0) data ~= -1;
              else data ~= 0;
            }
          else assert(isUlong || isLong);
        }
      else if(to.isFloat)
        {
          assert(isNumerical);

          float *fptr = cast(float*)data.ptr;

          if(isInt) *fptr = data[0];
          else if(isUint) *fptr = cast(uint)data[0];
          else if(isLong) *fptr = *(cast(long*)data.ptr);
          else if(isUlong) *fptr = *(cast(ulong*)data.ptr);
          else if(isDouble) *fptr = *(cast(double*)data.ptr);
          else assert(0);
          data = data[0..1];
        }
      else if(to.isDouble)
        {
          assert(isNumerical);

          if(data.length < 2) data.length = 2;
          double *fptr = cast(double*)data.ptr;

          if(isInt) *fptr = data[0];
          else if(isUint) *fptr = cast(uint)data[0];
          else if(isLong) *fptr = *(cast(long*)data.ptr);
          else if(isUlong) *fptr = *(cast(ulong*)data.ptr);
          else if(isFloat) *fptr = *(cast(float*)data.ptr);
          else assert(0);
          data = data[0..2];
        }
      else
        fail("Compile time conversion " ~ toString ~ " to " ~ to.toString ~
             " not implemented.");

      assert(data.length == toSize);
      return data;
    }

  int getSize()
    {
      if( isInt || isUint || isFloat || isChar || isBool )
        return 1;

      if( isLong || isUlong || isDouble )
        return 2;

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
    // Floats default to not a number
    else if(isFloat) data = makeData!(float)(float.nan);
    else if(isDouble) data = makeData!(double)(double.nan);
    else
      assert(0, "Type '" ~ name ~ "' has no default initializer");

    assert(data.length == getSize, "size mismatch in defaultInit");

    return data;
  }
}

// Represents a normal class name. The reason this is called
// "ObjectType" is because an actual variable (of this type) points to
// an object, not to a class. The meta-type ClassType should be used
// for variables that point to classes.
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

  MonsterClass getClass()
    {
      assert(clsIndex != 0);

      if(!global.isLoaded(clsIndex))
        fail("Cannot use " ~ name ~
             ": class not found or forward reference", loc);

      return global.getClass(clsIndex);
    }

 override:
  // getClass does all the error checking we need
  void validate() { getClass(); }

  int getSize() { return 1; }
  bool isObject() { return true; }
  int[] defaultInit() { return makeData!(int)(0); }

  bool canCastTo(Type type)
    {
      assert(clsIndex != 0);

      if(type.isString) return true;

      if(type.isObject)
        {
          auto ot = cast(ObjectType)type;
          assert(ot !is null);

          MonsterClass us = getClass();
          MonsterClass other = ot.getClass();

          assert(us != other);

          // We can only upcast
          return other.parentOf(us);
        }

      return false;
    }

  void evalCastTo(Type to)
    {
      assert(clsIndex != 0);
      assert(canCastTo(to));

      if(to.isObject)
        {
          auto tt = cast(ObjectType)to;
          assert(tt !is null);
          assert(clsIndex !is tt.clsIndex);
          int cnum = tt.clsIndex;

          tasm.upcast(cnum);
          return;
        }

      assert(to.isString);
      tasm.castObjToString();
    }

  // Members of objects are resolved in the class scope.
  Scope getMemberScope()
    {
      return getClass().sc;
    }

  // This is called when the type is defined, and it can forward
  // reference classes that do not exist yet. The classes must exist
  // by the time getClass() is called though, usually when class body
  // (not just the header) is being resolved.
  void resolve(Scope sc)
    {
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

  override:
  void validate() { assert(base !is null); base.validate(); }
  int arrays() { return base.arrays() + 1; }
  int getSize() { return 1; }
  int[] defaultInit() { return makeData!(int)(0); }
  Type getBase() { return base; }

  Scope getMemberScope()
    {
      return ArrayProperties.singleton;
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
    }
}

class EnumType : Type
{
  // The scope contains the actual enum values
  EnumScope sc;

  this(EnumDeclaration ed)
    {
      name = ed.name.str;
      loc = ed.name.loc;
    }

  int[] defaultInit() { return [0]; }
  int getSize() { return 1; }

  void resolve(Scope sc) {}

  // can cast to int and to string, but not back

  // Scope getMemberScope() { return sc; }
  Scope getMemberScope()
    { return GenericProperties.singleton; }
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
  void validate()
    {
      foreach(v; vars)
        v.type.validate();
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

  Scope getMemberScope()
    { return GenericProperties.singleton; }
  // Scope getMemberScope() { return sc; }
}

/*
// A type that mimics another after it is resolved. Used in cases
// where we can't know the real type until the resolve phase.
abstract class Doppelganger : Type
{
 private:
  bool resolved;

  Type realType;

 public:
 override:
  int getSize()
    {
      assert(resolved);
      return realType.getSize();
    }
}
*/

// A 'delayed lookup' type - can replace itself after resolve is
// called.
// OK - SCREW THIS. Make a doppelganger instead.
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
  Token id;

 public:

  static bool canParse(TokenArray toks)
    {
      if(!isNext(toks, TT.Identifier)) return false;
      return true;
    }

  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Identifier, id);

      // Get the name and the line from the token
      name = id.str~"(replacer)";
      loc = id.loc;
    }

  void resolve(Scope sc)
    {
      realType = sc.findStruct(id.str);

      if(realType is null)
        // Not a struct. Maybe an enum?
        realType = sc.findEnum(id.str);

      if(realType is null)
        // Default to class name if nothing else is found. We can't
        // really check this here since it might be a forward
        // reference. These are handled later on.
        realType = new ObjectType(id);

      realType.resolve(sc);

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
  this() { name = "var"; }

  static bool canParse(TokenArray toks)
    { return isNext(toks, TT.Var); }

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

  void evalCastTo(Type to)
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

/*
  Types we might add later:

  TableType - a combination of lists, structures, hashmaps and
              arrays. Loosely based on Lua tables, but not the same.

  FunctionType - pointer to a function with a given header. Since all
                 Monster functions are object members (methods), all
                 function pointers are in effect delegates.

*/
