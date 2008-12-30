/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (block.d) is part of the Monster script language
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

module monster.compiler.block;

import monster.compiler.tokenizer;
import monster.compiler.scopes;
import monster.compiler.assembler;

import monster.vm.error;

// Base class for all kinds of blocks. A 'block' is a token or
// collection of tokens that belong together and form a syntactical
// unit. A block might for example be a statement, a declaration, a
// block of code, or the entire class.
abstract class Block
{
 protected:
  static Token next(ref TokenArray toks)
    {
      if(toks.length == 0)
	fail("Unexpected end of file");
      Token tt = toks[0];
      toks = toks[1..toks.length];
      return tt;
    }

  static Floc getLoc(TokenArray toks)
    {
      Floc loc;
      if(toks.length) loc = toks[0].loc;
      return loc;
    }

  static bool isNext(ref TokenArray toks, TT type)
    {
      Floc ln;
      return isNext(toks, type, ln);
    }

  static bool isNext(ref TokenArray toks, TT type, out Floc loc)
    {
      if( toks.length == 0 ) return false;
      loc = toks[0].loc;
      if( toks[0].type != type ) return false;
      next(toks);
      return true;
    }

  static bool isNext(ref TokenArray toks, TT type, out Token tok)
    {
      if( toks.length == 0 ) return false;
      if( toks[0].type != type ) return false;
      tok = next(toks);
      return true;
    }

  static void reqNext(ref TokenArray toks, TT type, out Token tok)
    {
      if(!isNext(toks, type, tok))
         fail("Expected " ~ tokenList[type], toks);
    }

  static void reqNext(ref TokenArray toks, TT type, out Floc loc)
    {
      Token t;
      reqNext(toks, type, t);
      loc = t.loc;
    }

  static void reqNext(ref TokenArray toks, TT type)
    {
      Token t;
      reqNext(toks, type, t);
    }

  // Sets the assembler debug line to the line belonging to this
  // block.
  final void setLine() { tasm.setLine(loc.line); }

 public:
  // File position where this block was defined
  Floc loc;

  // Parse a list of tokens and attempt to understand how they belong
  // together. This is the syntactical level.
  void parse(ref TokenArray toks);

  // This goes through the code, resolves names and types etc,
  // converts expressions into an intermediate form which can be
  // compiled to byte code later. This is basically the semantic level.
  void resolve(Scope sc);
}
