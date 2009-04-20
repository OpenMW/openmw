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
import monster.compiler.expression;
import monster.compiler.statement;
import monster.compiler.tokenizer;

import monster.vm.error;

// Definition of a field
struct FieldDef
{
  Token name;
  Type type;
}

// Definition of each entry in the enum
struct EnumEntry
{
  Token name; // Entry identifier
  int index;  // Enum index value
  char[] stringValue; // Returned when printing the value

  long value; // Numeric value assigned to the enum. Can be set by the
              // user, but defaults to 0 for the first entry and to
              // the previous entry+1 when no value is set.

  Expression exp[]; // Field values (before resolving)
  int[][] fields; // Field values
}

class EnumDeclaration : TypeDeclaration
{
  static bool canParse(TokenArray toks)
    { return toks.isNext(TT.Enum); }

  EnumType type;

  override:
  void parse(ref TokenArray toks)
    {
      type = new EnumType();

      reqNext(toks, TT.Enum);
      reqNext(toks, TT.Identifier, type.nameTok);

      // Field definitions?
      while(isNext(toks, TT.Colon))
        {
          FieldDef fd;
          fd.type = Type.identify(toks);
          reqNext(toks, TT.Identifier, fd.name);
          type.fields ~= fd;
        }

      reqNext(toks, TT.LeftCurl);

      type.name = type.nameTok.str;
      type.loc = type.nameTok.loc;

      // Parse the entries and their fields
      int lastVal = -1;
      while(!isNext(toks, TT.RightCurl))
        {
          EnumEntry entry;
          reqNext(toks, TT.Identifier, entry.name);

          // Get the given value, if any
          if(isNext(toks, TT.Equals))
            {
              Token num;
              reqNext(toks, TT.IntLiteral, num);
              lastVal = LiteralExpr.parseIntLiteral(num);
            }
          else
            // No given value, just increase the last one given
            lastVal++;

          entry.value = lastVal;

          while(isNext(toks, TT.Colon))
            entry.exp ~= Expression.identify(toks);

          reqSep(toks, TT.Comma);

          type.entries ~= entry;
        }

      if(type.entries.length == 0)
        fail("Enum is empty", type.loc);

      isNext(toks, TT.Semicolon);
    }

  void insertType(TFVScope last)
    {
      // Insert ourselves into the parent scope
      assert(last !is null);
      assert(type !is null);
      last.insertEnum(type);
    }

  void resolve(Scope last)
    {
      // Delegate to the type, since all the variables are defined
      // there.
      type.resolve(last);
    }
}
