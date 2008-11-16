/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (tokenizer.d) is part of the Monster script language
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

module monster.compiler.tokenizer;

import std.string;
import std.stream;
import std.stdio;

import monster.util.string : begins;

import monster.vm.error;

alias Token[] TokenArray;

// Check if a character is alpha-numerical or an underscore
bool validIdentChar(char c)
{
  if(validFirstIdentChar(c) || numericalChar(c))
    return true;
  return false;
}

// Same as above, except numbers are not allowed as the first
// character. Will extend to support UTF8 later.
bool validFirstIdentChar(char c)
{
  if((c >= 'a' && c <= 'z') ||
     (c >= 'A' && c <= 'Z') ||
     (c == '_') ) return true;
  return false;
}

bool numericalChar(char c)
{
  return c >= '0' && c <= '9';
}

enum TT
  {
    // Syntax characters
    Semicolon, DDDot, DDot,
    LeftParen, RightParen,
    LeftCurl, RightCurl,
    LeftSquare, RightSquare,
    Dot, Comma, Colon,

    // Array length symbol
    Dollar,

    // Conditional expressions
    IsEqual, NotEqual,
    IsCaseEqual, IsCaseEqual2,
    NotCaseEqual, NotCaseEqual2,
    Less, More,
    LessEq, MoreEq,
    And, Or, Not,

    // Assignment operators.
    Equals, PlusEq, MinusEq, MultEq, DivEq, RemEq, IDivEq,
    CatEq,

    // Pre- and postfix increment and decrement operators ++ --
    PlusPlus, MinusMinus,

    // Arithmetic operators
    Plus, Minus, Mult, Div, Rem, IDiv,
    Cat,

    // Keywords. Note that we use Class as a separator below, so it
    // must be first in this list. All operator tokens must occur
    // before Class, and all keywords must come after Class.
    Class,
    For,
    If,
    Else,
    Foreach,
    ForeachRev,
    Do,
    While,
    Until,
    Typeof,
    Return,
    Continue,
    Break,
    Switch,
    Select,
    State,
    Singleton, Clone,
    This, New, Static, Const, Out, Ref, Abstract, Idle,
    Public, Private, Protected, True, False, Native, Null,
    Goto, Halt, Auto, Var, In,

    Last, // Tokens after this do not have a specific string
	  // associated with them.

    StringLiteral,	// "something"
    NumberLiteral,	// Anything that starts with a number
    CharLiteral,	// 'a'
    Identifier,		// user-named identifier
    EOF			// end of file
  }


struct Token
{
  TT type;
  char[] str;
  Floc loc;

  char[] toString() { return str; }
}

// Used to look up keywords.
TT keywordLookup[char[]];

void initTokenizer()
{
  // Insert the keywords into the lookup table
  for(TT t = TT.Class; t < TT.Last; t++)
    {
      char[] tok = tokenList[t];
      assert(tok != "");
      assert((tok in keywordLookup) == null);
      keywordLookup[tok] = t;
    }
}

// Index table of all the tokens
const char[][] tokenList =
  [
    TT.Semicolon        : ";",
    TT.DDDot            : "...",
    TT.DDot             : "..",
    TT.LeftParen        : "(",
    TT.RightParen       : ")",
    TT.LeftCurl         : "{",
    TT.RightCurl        : "}",
    TT.LeftSquare       : "[",
    TT.RightSquare      : "]",
    TT.Dot              : ".",
    TT.Comma            : ",",
    TT.Colon            : ":",

    TT.Dollar           : "$",

    TT.IsEqual          : "==",
    TT.NotEqual         : "!=",
    TT.IsCaseEqual      : "=i=",
    TT.IsCaseEqual2     : "=I=",
    TT.NotCaseEqual     : "!=i=",
    TT.NotCaseEqual2    : "!=I=",
    TT.Less             : "<",
    TT.More             : ">",
    TT.LessEq           : "<=",
    TT.MoreEq           : ">=",
    TT.And              : "&&",
    TT.Or               : "||",
    TT.Not              : "!",

    TT.Equals           : "=",
    TT.PlusEq           : "+=",
    TT.MinusEq          : "-=",
    TT.MultEq           : "*=",
    TT.DivEq            : "/=",
    TT.RemEq            : "%%=",
    TT.IDivEq           : "\\=",
    TT.CatEq            : "~=",

    TT.PlusPlus         : "++",
    TT.MinusMinus       : "--",
    TT.Cat              : "~",

    TT.Plus             : "+",
    TT.Minus            : "-",
    TT.Mult             : "*",
    TT.Div              : "/",
    TT.Rem              : "%%",
    TT.IDiv             : "\\",

    TT.Class            : "class",
    TT.Return           : "return",
    TT.For              : "for",
    TT.This             : "this",
    TT.New              : "new",
    TT.If               : "if",
    TT.Else             : "else",
    TT.Foreach          : "foreach",
    TT.ForeachRev       : "foreach_reverse",
    TT.Do               : "do",
    TT.While            : "while",
    TT.Until            : "until",
    TT.Continue         : "continue",
    TT.Break            : "break",
    TT.Switch           : "switch",
    TT.Select           : "select",
    TT.State            : "state",
    TT.Typeof           : "typeof",
    TT.Singleton        : "singleton",
    TT.Clone            : "clone",
    TT.Static           : "static",
    TT.Const            : "const",
    TT.Abstract         : "abstract",
    TT.Idle             : "idle",
    TT.Out 	        : "out",
    TT.Ref	        : "ref",
    TT.Public 	        : "public",
    TT.Private          : "private",
    TT.Protected        : "protected",
    TT.True             : "true",
    TT.False 	        : "false",
    TT.Native           : "native",
    TT.Null             : "null",
    TT.Goto             : "goto",
    TT.Halt             : "halt",
    TT.Auto             : "auto",
    TT.Var              : "var",
    TT.In               : "in",
  ];

class StreamTokenizer
{
 private:
  // Line buffer. Don't worry, this is perfectly safe. It is used by
  // Stream.readLine, which uses the buffer if it fits and creates a
  // new one if it doesn't. It is only here to optimize memory usage
  // (avoid creating a new buffer for each line), and lines longer
  // than 300 characters will work without problems.
  char[300] buffer;
  char[] line; // The rest of the current line
  Stream inf;
  uint lineNum;
  char[] fname;

  // Make a token of given type with given string, and remove it from
  // the input line.
  Token retToken(TT type, char[] str)
    {
      Token t;
      t.type = type;
      t.str = str;
      t.loc.fname = fname;
      t.loc.line = lineNum;

      // Special case for =I= and !=I=. Treat them the same as =i= and
      // !=i=.
      if(type == TT.IsCaseEqual2) t.type = TT.IsCaseEqual;
      if(type == TT.NotCaseEqual2) t.type = TT.NotCaseEqual;

      // Remove the string from 'line', along with any following witespace
      remWord(str);
      return t;
    }

  // Removes 'str' from the beginning of 'line', or from
  // line[leadIn..$] if leadIn != 0.
  void remWord(char[] str, int leadIn = 0)
    {
      assert(line.length >= leadIn);
      line = line[leadIn..$];

      assert(line.begins(str));
      line = line[str.length..$].stripl();
    }

  Token eofToken()
    {
      Token t;
      t.str = "<end of file>";
      t.type = TT.EOF;
      t.loc.line = lineNum;
      t.loc.fname = fname;
      return t;
    }

 public:
 final:
  this(char[] fname, Stream inf, int bom)
    {
      assert(inf !is null);

      // The BOM (byte order mark) defines the byte order (little
      // endian or big endian) and the encoding (utf8, utf16 or
      // utf32).
      switch(bom)
        {
        case -1:
          // Files without a BOM are interpreted as UTF8
        case BOM.UTF8:
          // UTF8 is the default
          break;

        case BOM.UTF16LE:
        case BOM.UTF16BE:
        case BOM.UTF32LE:
        case BOM.UTF32BE:
          fail("UTF16 and UTF32 files are not supported yet");
        default:
          fail("Unknown BOM value!");
        }

      this.inf = inf;
      this.fname = fname;
    }

  ~this() { if(inf !is null) delete inf; }

  void fail(char[] msg)
    {
      throw new MonsterException(format("%s:%s: %s", fname, lineNum, msg));
    }

  Token getNext()
    {
      // Various parsing modes
      enum
	{
	  Normal,	// Normal mode
	  Block,	// Block comment
	  Nest		// Nested block comment
	}
      int mode = Normal;
      int nests = 0;    // Nest level

    restart:
      // Get the next line, if the current is empty
      while(line.length == 0)
	{
	  // No more information, we're done
	  if(inf.eof())
	    {
	      if(mode == Block) fail("Unterminated block comment");
	      if(mode == Nest) fail("Unterminated nested comment");
	      return eofToken();
	    }

	  // Read a line and remove leading and trailing whitespace
	  line = inf.readLine(buffer).strip();
	  lineNum++;
	}

      assert(line.length > 0);

      // Skip the first line if it begins with #!
      if(lineNum == 1 && line.begins("#!"))
          {
            line = null;
            goto restart;
          }

      if(mode == Block)
	{
	  int index = line.find("*/");

	  // If we find a '*/', the comment is done
	  if(index != -1)
	    {
	      mode = Normal;

	      // Cut it the comment from the input
	      remWord("*/", index);
	    }
	  else
	    {
	      // Comment not ended on this line, try the next
	      line = null;
	    }

	  // Start over
	  goto restart;
	}

      if(mode == Nest)
	{
	  // Check for nested /+ and +/ in here, but go to restart if
	  // none is found (meaning the comment continues on the next
	  // line), or reset mode and go to restart if nest level ever
	  // gets to 0.

	  do
	    {
	      int incInd = -1;
	      int decInd = -1;
	      // Find the first matching '/+' or '+/
	      foreach(int i, char c; line[0..$-1])
		{
		  if(c == '/' && line[i+1] == '+')
		    {
		      incInd = i;
		      break;
		    }
		  else if(c == '+' && line[i+1] == '/')
		    {
		      decInd = i;
		      break;
		    }
		}

	      // Add a nest level when '/+' is found
	      if(incInd != -1)
		{
		  remWord("/+", incInd);
		  nests++;
		  continue; // Search more in this line
		}

	      // Remove a nest level when '+/' is found
	      if(decInd != -1)
		{
		  // Remove the +/ from input
		  remWord("+/", decInd);

		  nests--; // Remove a level
		  assert(nests >= 0);

		  // Are we done? If so, return to normal mode.
		  if(nests == 0)
		    {
		      mode = Normal;
		      break;
		    }
		  continue;
		}

	      // Nothing found on this line, try the next
	      line = null;
	      break;
	    }
	  while(line.length >= 2);

	  goto restart;
	}

      // Comment - start next line
      if(line.begins("//"))
	{
	  line = null;
	  goto restart;
	}

      // Block comment
      if(line.begins("/*"))
	{
	  mode = Block;
	  line = line[2..$];
	  goto restart;
	}

      // Nested comment
      if(line.begins("/+"))
	{
	  mode = Nest;
	  line = line[2..$];
	  nests++;
	  goto restart;
	}

      if(line.begins("*/")) fail("Unexpected end of block comment");
      if(line.begins("+/")) fail("Unexpected end of nested comment");

      // String literals (multi-line literals not implemented yet)
      if(line.begins("\""))
	{
	  int len = 1;
	  bool found = false;
	  foreach(char ch; line[1..$])
	    {
	      len++;
	      // No support for escape sequences as of now
	      if(ch == '"')
		{
		  found = true;
		  break;
		}
	    }
	  if(!found) fail("Unterminated string literal '" ~line~"'");
	  return retToken(TT.StringLiteral, line[0..len].dup);
	}

      // Character literals (not parsed yet, so escape sequences like
      // '\n', '\'', or unicode stuff won't work.)
      if(line[0] == '\'')
	{
	  if(line.length < 2 || line[2] != '\'')
	    fail("Malformed character literal " ~line);
	  return retToken(TT.CharLiteral, line[0..3].dup);
	}

      // Numerical literals - if it starts with a number, we accept
      // it, until it is interupted by an unacceptible character. We
      // also accept numbers on the form .NUM. We do not try to parse
      // the number here.
      if(numericalChar(line[0]) ||
	 // Cover the .num case
	( line.length >= 2 && line[0] == '.' &&
	  numericalChar(line[1])             ))
	{
	  // Treat the rest as we would an identifier - the actual
	  // interpretation will be done later. We allow non-numerical
	  // tokens in the literal, such as 0x0a or 1_000_000. We must
	  // also explicitly allow '.' dots. A number literal can end
	  // with a percentage sign '%'.
	  int len = 1;
	  bool lastDot = false; // Was the last char a '.'?
          bool lastPer = false; // Was it a '%'?
	  foreach(char ch; line[1..$])
	    {
	      if(ch == '.')
		{
		  // We accept "." but not "..", as this might be an
		  // operator.
		  if(lastDot)
		    {
		      len--; // Remove the last dot and exit.
		      break;
		    }
		  lastDot = true;
		}
              else if(ch == '%')
                {
                  // Ditto for percentage signs. We allow '%' but not
                  // '%%'
                  if(lastPer)
                    {
                      len--;
                      break;
                    }
                  lastPer = true;
                }
	      else
		{
		  if(!validIdentChar(ch)) break;
		  lastDot = false;
                  lastPer = false;
		}

              // This was a valid character, count it
	      len++;
	    }
	  return retToken(TT.NumberLiteral, line[0..len].dup);
	}

      // Check for identifiers
      if(validFirstIdentChar(line[0]))
	{
	  // It's an identifier or name, find the length
	  int len = 1;
	  foreach(char ch; line[1..$])
	    {
	      if(!validIdentChar(ch)) break;
	      len++;
	    }

	  char[] id = line[0..len];

          // We only allow certain identifiers to begin with __, as
          // these are reserved for internal use.
          if(id.begins("__"))
            if(id != "__STACK__")
              fail("Identifier " ~ id ~ " is not allowed to begin with __");

	  // Check if this is a keyword
          if(id in keywordLookup)
            {
              TT t = keywordLookup[id];
              assert(t >= TT.Class && t < TT.Last,
                     "Found " ~ id ~ " as a keyword, but with wrong type!");
	      return retToken(t, tokenList[t]);
            }

	  // Not a keyword? Then it's an identifier
	  return retToken(TT.Identifier, id.dup);
	}

      // Check for operators and syntax characters. We browse through
      // the entire list, and select the longest match that fits (so
      // we don't risk matching "+" to "+=", for example.)
      TT match;
      int mlen = 0;
      foreach(int i, char[] tok; tokenList[0..TT.Class])
        if(line.begins(tok) && tok.length >= mlen)
          {
            assert(tok.length > mlen, "Two matching tokens of the same length");
            mlen = tok.length;
            match = cast(TT) i;
          }

      if(mlen) return retToken(match, tokenList[match]);

      // Invalid token
      fail("Invalid token " ~ line);
    }

  // Require a specific token
  bool isToken(TT tok)
    {
      Token tt = getNext();
      return tt.type == tok;
    }

  bool notToken(TT tok) { return !isToken(tok); }
}

// Read the entire file into an array of tokens. This includes the EOF
// token at the end.
TokenArray tokenizeStream(char[] fname, Stream stream, int bom)
{
  TokenArray tokenArray;

  StreamTokenizer tok = new StreamTokenizer(fname, stream, bom);
  Token tt;
  do
    {
      tt = tok.getNext();
      tokenArray ~= tt;
    }
  while(tt.type != TT.EOF)
  delete tok;

  return tokenArray;
}
