/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (structs.d) is part of the Monster script language
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

module monster.compiler.structs;

import monster.compiler.types;
import monster.compiler.scopes;
import monster.compiler.variables;
import monster.compiler.functions;
import monster.compiler.tokenizer;
import monster.compiler.statement;
import monster.vm.error;
import std.stdio;

class StructDeclaration : TypeDeclaration
{
  StructType type;
  Token name;

 private:
  FuncDeclaration[] funcdecs;
  VarDeclStatement[] vardecs;

  // Identify what kind of block the given set of tokens represent,
  // parse them, and store it in the appropriate list;
  void store(ref TokenArray toks)
    {
      // canParse() is not ment as a complete syntax test, only to be
      // enough to identify which Block parser to apply.
      if(FuncDeclaration.canParse(toks))
	{
	  auto fd = new FuncDeclaration;
	  funcdecs ~= fd;
	  fd.parse(toks);
	}
      else if(VarDeclStatement.canParse(toks))
	{
	  auto vd = new VarDeclStatement;
	  vd.parse(toks);
	  vardecs ~= vd;
	}
      else
        fail("Illegal type or declaration", toks);
    }

 public:

  static bool canParse(TokenArray toks)
    { return toks.isNext(TT.Struct); }

  override:
  void parse(ref TokenArray toks)
    {
      if(!isNext(toks, TT.Struct, loc))
        fail("Internal error in StructDeclaration");

      if(!isNext(toks, TT.Identifier, name))
        fail("Expected struct name", toks);

      if(!isNext(toks, TT.LeftCurl))
        fail("Struct expected {", toks);

      // Parse the rest of the file
      while(!isNext(toks, TT.RightCurl))
        store(toks);

      // Allow an optional semicolon
      isNext(toks, TT.Semicolon);
    }

  void insertType(TFVScope last)
    {
      // Set up the struct type.
      type = new StructType(this);

      // Create a new scope
      type.sc = new StructScope(last, type);

      // Insert ourselves into the parent scope
      assert(last !is null);
      last.insertStruct(this);
    }

  void resolve(Scope last)
    {
      if(type.set)
        return;

      // Get the total number of variables.
      uint tot = 0;
      foreach(dec; vardecs)
        tot += dec.vars.length;

      // Must have at least one variable per declaration statement
      assert(tot >= vardecs.length);

      // Get one array containing all the variables
      Variable*[] vars = new Variable*[tot];
      int ind = 0;
      foreach(st; vardecs)
        foreach(dec; st.vars)
          vars[ind++] = dec.var;
      assert(ind == tot);

      // Store the variables in the StructType
      type.vars = vars;

      // Mark the size as "set" now.
      type.set = true;

      // Resolve
      foreach(dec; vardecs)
        dec.resolve(type.sc);

      // Calculate the struct size
      type.size = 0;
      foreach(t; vars)
        type.size += t.type.getSize();

      // Set up the init value
      ind = 0;
      int[] init = new int[type.getSize()];
      foreach(st; vardecs)
        foreach(dec; st.vars)
        {
          int si = dec.var.type.getSize();
          init[ind..ind+si] = dec.getCTimeValue();
          ind += si;
        }
      assert(ind == init.length);
      type.defInit = init;

      // Functions:

      // Disallow anything but normal functions (we can fix static and
      // native struct functions later.)

      // Struct resolve only resolves header information, it doesn't
      // resolve function bodies.
      assert(funcdecs.length == 0, "struct functions not supported yet");
      /*
      foreach(dec; funcdecs)
        type.sc.insertFunc(dec);
      */
    }
}
