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

      //writefln("Type is: %s", var.type);

      if(var.type.isReplacer)
        {
          //writefln("  (we're here!)");
          var.type = var.type.getBase();
        }

      //writefln("  now it is: %s", var.type);

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

              fail("Struct variables cannot be used inside the struct itself!",
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
	{
	  // If we are not a function parameter, we must get
	  // var.number from the scope.
	  if(sc.isClass())
	    // Class variable. Get a position in the data segment.
	    var.number = sc.addNewDataVar(var.type.getSize());
	  else
	    // We're a local variable. Ask the scope what number we
	    // should have.
	    var.number = sc.addNewLocalVar(var.type.getSize());
	}
      else assert(sc.isFunc());

      // Insert ourselves into the scope.
      sc.insertVar(var);
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

      // Type names are always static. However, isType will return
      // false for type names of eg. singletons, since these will not
      // resolve to a meta type.
      if(isType)
        return true;

      return false;
    }

  // TODO: isCTime - should be usable for static properties and members

  void parse(ref TokenArray toks)
    {
      name = next(toks);
      loc = name.loc;
    }

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

      if(name.type == TT.Clone)
        fail("Cannot use clone as a variable", name.loc);

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

      // We are not a variable. Our last chance is a type name.
      vtype = VType.Type;
      if(BasicType.isBasic(name.str))
        {
          // Yes! Basic type.
          type = MetaType.getBasic(name.str);
        }
      // Class name?
      else if(auto mc = global.findParsed(name.str))
        {
          // This doesn't allow forward references.
          mc.requireScope();
          type = mc.classType;

          // Singletons are treated differently - the class name can
          // be used to access the singleton object
          if(mc.isSingleton)
            {
              type = mc.objType;
              singCls = mc.getIndex();
            }
        }
      // Struct?
      else if(auto tp = sc.findStruct(name.str))
        {
          type = tp.getMeta();
        }
      // Err, enum?
      else if(auto tp = sc.findEnum(name.str))
        {
          type = tp.getMeta();
        }
      else
        // No match at all
        fail("Undefined identifier "~name.str, name.loc);
    }

  void evalAsm()
    {
      assert(!isType);

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
