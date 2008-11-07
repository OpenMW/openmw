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
import monster.compiler.block;

import std.string;
import std.stdio;
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

  // Special version used for explicitly numbering function
  // parameters. Only called from FuncDeclaration.resolve()
  void resolve(Scope sc, int num)
    {
      assert(num<0, "VarDec.resolve was given a positive num: " ~ .toString(num));
      var.number = num;
      resolve(sc);
    }

  // Calls resolve() for all sub-expressions
  override void resolve(Scope sc)
    {
      var.type.resolve(sc);

      if(!allowConst && var.isConst)
        fail("'const' is not allowed here", loc);

      // Store the scope in the var struct for later referral.
      var.sc = cast(VarScope)sc;
      assert(var.sc !is null, "variables can only be declared in VarScopes");

      if(var.number == 0)
	{
	  // If 'number' has not been set at this point (ie. we are
	  // not a function parameter), we must get it from the scope.
	  if(sc.isClass())
	    // Class variable. Get a position in the data segment.
	    var.number = sc.addNewDataVar(var.type.getSize());
	  else
	    // We're a local variable. Ask the scope what number we
	    // should have.
	    var.number = sc.addNewLocalVar(var.type.getSize());
	}
      else assert(sc.isFunc());

      if(init !is null)
	{
          init.resolve(sc);

          // Convert type, if necessary.
          try var.type.typeCast(init);
          catch(TypeException)
	    fail(format("Cannot initialize %s of type %s with %s of type %s",
			var.name.str, var.type,
			init, init.type), loc);
	  assert(init.type == var.type);
	}

      // Insert ourselves into the scope.
      sc.insertVar(var);
    }

  // Executed for local variables upon declaration. Push the variable
  // on the stack.
  void compile()
    {
      // Validate the type
      var.type.validate();

      setLine();

      if(init !is null)
        // Push the initializer
	init.eval();
      else
        // Default initializer
        var.type.pushInit();
    }
}
