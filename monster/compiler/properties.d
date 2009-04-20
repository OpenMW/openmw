/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (properties.d) is part of the Monster script language
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

module monster.compiler.properties;
import monster.compiler.scopes;
import monster.compiler.types;
import monster.compiler.assembler;

import std.stdio;

/* This module contains special scopes for builtin types. These are
   used to resolve type properties like .length for arrays and .min
   and .max for ints, etc.
*/

// TODO: This is nice, but could be nicer. I would like most of these
// stored as values rather than functions, and have a general
// mechanism for converting values into data (a converted version of
// pushInit in Type) used for all types except the ones requiring a
// function. I guess I could make versions of insert(s) that takes
// floats, ints etc as parameters, and double checks it against the
// type. The main reason for storing values is that most of them can
// then be optimized away as compile time constants. This is not a
// priority.

class NumericProperties(T) : SimplePropertyScope
{
  this()
    {
      super(T.stringof ~ "Properties",
            GenericProperties.singleton);

      // Static properties of all numeric types
      static if(T.sizeof == 4)
        {
          inserts("min", T.stringof, { tasm.push(T.min); });
          inserts("max", T.stringof, { tasm.push(T.max); });
        }
      else static if(T.sizeof == 8)
        {
          inserts("min", T.stringof, { tasm.push8(T.min); });
          inserts("max", T.stringof, { tasm.push8(T.max); });
        }
      else static assert(0);
    }
}

class IntProperties : NumericProperties!(int)
{ static IntProperties singleton; }

class UintProperties : NumericProperties!(uint)
{ static UintProperties singleton; }

class LongProperties : NumericProperties!(long)
{ static LongProperties singleton; }

class UlongProperties : NumericProperties!(ulong)
{ static UlongProperties singleton; }

class FloatingProperties(T) : NumericProperties!(T)
{
 this()
    {
      char[] tp = T.stringof;

      // Additional static properties of floating point numbers
      static if(T.sizeof == 4)
        {
          inserts("infinity", tp, { tasm.push(T.infinity); });
          inserts("inf", tp, { tasm.push(T.infinity); });
          inserts("nan", tp, { tasm.push(T.nan); });
          inserts("epsilon", tp, { tasm.push(T.epsilon); });
        }
      else static if(T.sizeof == 8)
        {
          inserts("infinity", tp, { tasm.push8(T.infinity); });
          inserts("inf", tp, { tasm.push8(T.infinity); });
          inserts("nan", tp, { tasm.push8(T.nan); });
          inserts("epsilon", tp, { tasm.push8(T.epsilon); });
        }
      else static assert(0);

      inserts("dig", "int", { tasm.push(T.dig); });
      inserts("max_10_exp", "int", { tasm.push(T.max_10_exp); });
      inserts("max_exp", "int", { tasm.push(T.max_exp); });
      inserts("min_10_exp", "int", { tasm.push(T.min_10_exp); });
      inserts("min_exp", "int", { tasm.push(T.min_exp); });

      // Number of bits in mantissa. D calls it mant_dig, but
      // mant_bits is more natural. Let us allow both.
      inserts("mant_bits", "int", { tasm.push(T.mant_dig); });
      inserts("mant_dig", "int", { tasm.push(T.mant_dig); });

      // Lets add in number of bits in the exponent as well.
      inserts("exp_bits", "int", { tasm.push(cast(uint)(8*T.sizeof-T.mant_dig)); });
    }
}

class FloatProperties : FloatingProperties!(float)
{ static FloatProperties singleton; }

class DoubleProperties : FloatingProperties!(double)
{ static DoubleProperties singleton; }

// Handles .length, .dup, etc for arrays
class ArrayProperties: SimplePropertyScope
{
  static ArrayProperties singleton;

  this()
    {
      super("ArrayProperties", GenericProperties.singleton);

      insert("length", "int",
          { tasm.getArrayLength(); },
          { assert(0, "cannot set length yet"); });
      insert("dup", "owner", { tasm.dupArray(); });
      insert("reverse", "owner", { tasm.reverseArray(); });
      insert("sort", "owner", { assert(0, "sort not implemented"); });
      insert("const", "owner", { tasm.makeArrayConst(); });
      insert("isConst", "bool", { tasm.isArrayConst(); });
    }
}

// TODO: Rename to ObjectProperties
class ClassProperties : SimplePropertyScope
{
  static ClassProperties singleton;

  this()
    {
      super("ClassProperties",
            GenericProperties.singleton);

      insert("clone", "owner", { tasm.cloneObj(); });

      // We should move handling of states here. This will mean
      // removing StateStatement and making states a propert type. We
      // can't leave statestatement in as a special syntax for setting
      // types, because the member syntax obj.state = state.label;
      // would still have to be handled somehow. However, even if this
      // is more work, it has some additional benefits of allowing
      // states to be used in other expressions, eg state ==
      // SomeState. And we should still be able to optimize it into
      // one instruction.

      // One downside now is that we are currently using static
      // properties. If we are going to use non-static properties and
      // allow both member and non-member access, we have to
      // differentiate between near and far properties too. Think more
      // about it.
      //insert("state", "int", { tasm.push(6); });
    }
}

// Dynamically handles properties like init and sizeof that are valid
// for all types.
class GenericProperties : SimplePropertyScope
{
  static GenericProperties singleton;

  this()
    {
      super("GenericProperties");

      inserts("init", "owner", {assert(0);});
      inserts("sizeof", "int", {assert(0);});
      inserts("bitsof", "int", {assert(0);});
    }

  // Overwrite the above actions
  void getValue(char[] name, Type oType)
    {
      if(oType.isMeta) oType = oType.getBase();

      if(name == "sizeof") tasm.push(oType.getSize);
      else if(name == "bitsof") tasm.push(oType.getSize*32);
      else if(name == "init") oType.pushInit();
      else assert(0);
    }
}

/* This is a base class that simplifies definition of property
   scopes. You can simply call insert and inserts (for static
   properties) in the constructor. An example:

   inserts("max", "int", { tasm.push(int.max); });

   This inserts the property "max", of type "int", and pushes the
   value int.max whenever it is invoked. Since it is a static
   property, the left hand side (eg an int value) is never
   evaluated. If the type is "" or "owner", then the property type
   will be the same as the owner type.
 */

abstract class SimplePropertyScope : PropertyScope
{
  this(char[] n, PropertyScope ps = null) { super(n, ps); }

  private SP[char[]] propList;

  // Convert a typename to a type
  private Type getType(char[] tp)
    {
      if(tp == "" || tp == "owner")
        return null;

      return BasicType.get(tp);
    }

  // Insert properties into the list
  void insert(char[] name, Type tp, Action push, Action pop = null)
    {
      assert(!hasProperty(name));
      propList[name] = SP(tp, false, push, pop);
    }

  void insert(char[] name, char[] tp, Action push, Action pop = null)
    { insert(name, getType(tp), push, pop); }

  // Insert static properties. TODO: These should take values rather
  // than code. It should be possible to retireve this value at
  // compile-time.
  void inserts(char[] name, Type tp, Action push)
    {
      assert(!hasProperty(name));
      propList[name] = SP(tp, true, push, null);
    }

  void inserts(char[] name, char[] tp, Action push)
    { inserts(name, getType(tp), push); }

  // TODO: These are hacks to work around a silly but irritating DMD
  // feature. Whenever there's an error somewhere, function literals
  // like { something; } get resolved as int delegate() instead of
  // void delegate() for some reason. This gives a ton of error
  // messages, but these overloads will prevent that.
  alias int delegate() FakeIt;
  void insert(char[],char[],FakeIt,FakeIt pop = null) {assert(0);}
  void insert(char[],Type,FakeIt,FakeIt pop = null) {assert(0);}
  void inserts(char[],char[],FakeIt) {assert(0);}
  void inserts(char[],Type,FakeIt) {assert(0);}

 override:

  // Return the stored type. If it is null, return the owner type
  // instead.
  Type getType(char[] name, Type oType)
    {
      assert(hasProperty(name));
      Type tp = propList[name].type;

      // No stored type? We have to copy the owner.
      if(tp is null)
        {
          // The owner type might be a meta-type (eg. int.init). Pretend
          // it is the base type instead.
          if(oType.isMeta()) 
            tp = oType.getBase();
          else
            tp = oType;
        }

      return tp;
    }

  void getValue(char[] name, Type oType)
    {
      assert(hasProperty(name));
      propList[name].push();
    }

  void setValue(char[] name, Type oType)
    {
      assert(hasProperty(name));
      propList[name].pop();
    }

  bool hasProperty(char[] name)
    { return (name in propList) != null; }

  bool isStatic(char[] name, Type oType)
    {
      assert(hasProperty(name));
      return propList[name].isStatic;
    }

  bool isLValue(char[] name, Type oType)
    {
      assert(hasProperty(name));
      return propList[name].isLValue();
    }
}

alias void delegate() Action;
struct SP
{
  Action push, pop;
  Type type;
  bool isStatic;

  static SP opCall(Type tp, bool stat, Action push, Action pop)
  {
    SP s;
    s.push = push;
    s.pop = pop;
    s.type = tp;
    s.isStatic = stat;
    assert(!stat || pop == null);
    return s;
  }

  bool isLValue() { return pop != null; }
}

void initProperties()
{
  GenericProperties.singleton = new GenericProperties;
  ArrayProperties.singleton = new ArrayProperties;
  IntProperties.singleton = new IntProperties;
  UintProperties.singleton = new UintProperties;
  LongProperties.singleton = new LongProperties;
  UlongProperties.singleton = new UlongProperties;
  FloatProperties.singleton = new FloatProperties;
  DoubleProperties.singleton = new DoubleProperties;
  ClassProperties.singleton = new ClassProperties;
}
