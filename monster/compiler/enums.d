/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (enums.d) is part of the Monster script language package.

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

module monster.compiler.enums;

import monster.compiler.scopes;
import monster.compiler.types;
import monster.compiler.statement;
import monster.compiler.tokenizer;

class EnumDeclaration : TypeDeclaration
{
  static bool canParse(TokenArray toks)
    { return toks.isNext(TT.Enum); }

  Token name;
  EnumType type;

  override:
  void parse(ref TokenArray toks)
    {
      reqNext(toks, TT.Enum);
      reqNext(toks, TT.Identifier, name);
      reqNext(toks, TT.LeftCurl);

      // Just skip everything until the matching }. This lets us
      // define some enums and play around in the scripts, even if it
      // doesn't actually work.
      while(!isNext(toks, TT.RightCurl)) next(toks);

      isNext(toks, TT.Semicolon);
    }

  void insertType(TFVScope last)
    {
      type = new EnumType(this);

      // Insert ourselves into the parent scope
      assert(last !is null);
      last.insertEnum(this);
    }

  // Very little to resolve, really. It's purely a declarative
  // statement.
  void resolve(Scope last)
    {}
}
