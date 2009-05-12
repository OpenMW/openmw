/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (console.d) is part of the Monster script language package.

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

module monster.modules.console;

// This file implements an optimized way of running Monster
// interactively, what is sometimes called "line mode" or "interactive
// mode". It is ideally suited for making ingame consoles.

// The main idea is to retain all reusable data structures and
// minimize the number of heap allocations at runtime. The current
// implemention is not perfected in that regard, but later
// implementations will be.

// All input into the console is given through the input() function,
// and all output is sent to the output() callback function. You can
// also poll for output manually using output();

import monster.util.growarray;
import monster.compiler.tokenizer;
import monster.compiler.statement;
import monster.compiler.variables;
import monster.compiler.functions;
import monster.compiler.scopes;
import monster.compiler.bytecode;
import monster.compiler.assembler;
import monster.compiler.types;
import monster.compiler.expression;
import std.stdio;
import std.string;
import monster.monster;

// Console results
enum CR
  {
    Ok = 1,    // Command was executed
    Error = 2, // An error occurred
    More = 3,  // An unterminated multi-line statement was entered, need
               // more input
    Empty = 4, // The line was empty (nothing was executed)
  }

class Console
{
 private:
  Tokenizer tn;

  GrowArray!(Token) tokArr;
  GrowArray!(char) outBuf;

  Function fn;

  FuncScope sc;

  MonsterObject *obj;

  Variable* varList[];
  uint varSize;

  // The thread that we run console commands in. It's put in the
  // background when not in use.
  Thread *trd;

  // The thread that was running when we started (if any)
  Thread *store;

  int paren, curl, square;

  void delegate(char[] str) output_cb;
  bool hasCallback;

  char[] norm_prompt = ">>> ";
  int tab = 4;
  char[] ml_prompt = "... ";
  char[] cmt_prompt = "(comment) ";

 public:
  bool allowVar = true;

  this(MonsterObject *ob = null)
    {
      tn = new Tokenizer();

      // Set the context object
      obj = ob;
      if(obj is null)
        obj = Function.getIntMO();

      // Next set up the function and the scope
      fn.name.str = "console";
      fn.owner = obj.cls;
      sc = new FuncScope(obj.cls.sc, &fn);

      // Get a new thread
      trd = Thread.getPaused();
    }

  void put(char[] str, bool newLine=false)
    {
      if(hasCallback)
        {
          output_cb(str);
          if(newLine)
            output_cb("\n");
        }
      else
        {
          outBuf ~= str;
          if(newLine)
            outBuf ~= '\n';
        }
    }

  void putln(char[] str) { put(str, true); }

 private:

  Statement[] parse(TokenArray toks, Scope sc)
    {
      Statement b;
      Statement[] res;

    repeat:

      b = null;

      if(CodeBlock.canParse(toks)) b = new CodeBlock;
      else if(IfStatement.canParse(toks)) b = new IfStatement;
      else if(DoWhileStatement.canParse(toks)) b = new DoWhileStatement;
      else if(WhileStatement.canParse(toks)) b = new WhileStatement;
      else if(ForStatement.canParse(toks)) b = new ForStatement;
      else if(ForeachStatement.canParse(toks)) b = new ForeachStatement;
      else if(ImportStatement.canParse(toks)) b = new ImportStatement(true);

      if(b !is null)
        {
          // Parse and resolve
          b.parse(toks);
          b.resolve(sc);
        }
        else
        {
          // If this is not one of the above, default to a console
          // statement.
          auto es = new ConsoleStatement;
          b = es;

          // Parse and resolve in one operation.
          es.parseResolve(toks, sc, allowVar);
        }

      assert(b !is null);
      res ~= b;

      // Are there more tokens waiting for us?
      if(toks.length > 0)
        goto repeat;

      return res;
    }

  int sumParen()
    {
      // Clean up if we had an unmatched end bracket somewhere
      if(paren < 0) paren = 0;
      if(curl < 0) curl = 0;
      if(square < 0) square = 0;

      return paren + curl + square;
    }

  bool isComment()
    {
      return tn.mode != Tokenizer.Normal;
    }

  // Resets the console to a usable state. Does not delete variables.
  void reset()
    {
      paren = 0;
      curl = 0;
      square = 0;
      tn.mode = Tokenizer.Normal;

      if(cthread is trd)
        {
          // Reset the function stack.
          trd.fstack.killAll();

          assert(trd.fstack.isEmpty);
          assert(!trd.fstack.hasNatives);

          // Our variables should still be on the stack though.
          if(stack.getPos > varSize)
            stack.popInts(stack.getPos - varSize);
          else
            assert(stack.getPos == varSize);

          // Make sure the thread is still in the 'paused' mode
          trd.moveTo(&scheduler.paused);

          // Background the thread - this will also capture the stack
          trd.background();
        }

      assert(trd !is cthread);

      // Restore the previous thread (if any)
      if(store !is null)
        store.foreground();

      store = null;
    }

  // Push the input into the compiler and run it
  CR runInput(char[] str)
    {
      // Set up the tokenizer
      tn.setLine(str);

      // Reset the token buffer, unless we're in a multiline command
      if(paren == 0 && curl == 0 && square == 0)
        tokArr.length = 0;

      // Phase I, tokenize
      Token t = tn.getNextFromLine();
      // Mark the first token as a newline / separator
      t.newline = true;
      while(t.type != TT.EMPTY)
        {
          if(t.type == TT.LeftParen)
            paren++;
          else if(t.type == TT.LeftCurl)
            curl++;
          else if(t.type == TT.LeftSquare)
            square++;
          else if(t.type == TT.RightParen)
            paren--;
          else if(t.type == TT.RightCurl)
            curl--;
          else if(t.type == TT.RightSquare)
            square--;

          tokArr ~= t;
          t = tn.getNextFromLine();
        }

      if(paren < 0 || curl < 0 || square < 0)
        fail("Unmatched end bracket(s)");

      // Wait for more input inside a bracket
      if(sumParen() > 0)
        return CR.More;

      // Ditto for block comments
      if(isComment())
        return CR.More;

      // Ignore empty token lists
      if(tokArr.length == 0)
        return CR.Empty;

      // Phase II & III, parse and resolve
      TokenArray toks = tokArr.arrayCopy();
      Statement[] sts = parse(toks, sc);
      delete toks;
      assert(sts.length >= 1);

      // First, background the current thread (if any) and bring up
      // our own. This is necessary in order to keep the stack
      // variables we make.
      store = cthread;
      if(store !is null)
        store.background();
      assert(trd !is null);
      trd.foreground();

      // We have to push ourselves on the function stack, or
      // Function.call() will see that it's empty and kill the thread
      // upon exit
      trd.fstack.pushExt("Console");

      // The rest must be performed separately for each statement on
      // the line.
      foreach(st; sts)
        {
          // Phase IV, compile
          tasm.newFunc();

          ExprStatement es;

          // Is it an special console statement?
          auto cs = cast(ConsoleStatement)st;
          if(cs !is null)
            {
              // Yes. Is the client an expression?
              es = cast(ExprStatement)cs.client;
              if(es !is null)
                {
                  // Ok. But is the type usable?
                  if(es.left.type.canCastTo(ArrayType.getString())
                     && es.right is null)
                    {
                      // Yup. Get the type, and cast the expression to string.
                      scope auto ce = new
                        CastExpression(es.left, ArrayType.getString());
                      ce.eval();
                    }
                  else es = null;
                }
            }

          // No expression is being used, so compile the statement
          // normally.
          if(es is null)
            st.compile();

          // Gather all the statements into one function and get the
          // bytecode.
          fn.bcode = tasm.assemble(fn.lines);
          fn.bcode ~= cast(ubyte)BC.Exit; // Non-optimal hack

          // Phase V, call the function
          fn.call(obj);

          // Finally, get the expression result, if any, and print it.
          if(es !is null)
            putln(stack.popString8());

          // In the case of new a variable declaration, we have to
          // make sure they are accessible to any subsequent calls to
          // the function. Since the stack frame gets set at each
          // call, we have to access the variables outside the
          // function frame, ie. the same way we treat function
          // parameters. We do this by giving the variables negative
          // indices.
          if(cs !is null)
            {
              auto vs = cast(VarDeclStatement)cs.client;
              if(vs !is null)
                {
                  // Add the new vars to the list
                  foreach(v; vs.vars)
                    {
                      varList ~= v.var;

                      // Add the size as well
                      varSize += v.var.type.getSize;
                    }

                  // Recalculate all the indices backwards from zero
                  int place = 0;
                  foreach_reverse(v; varList)
                    {
                      place -= v.type.getSize;
                      v.number = place;
                    }

                  // Reset the scope stack counters
                  sc.reset();
                }
            }
        }

      trd.fstack.pop();

      // Reset the console to a usable state
      reset();

      return CR.Ok;
    }

 public:

  void prompt()
    {
      int sum = sumParen();
      bool isBracket = (sum != 0);

      sum = sum * tab + norm_prompt.length;

      if(isComment)
        {
          sum -= cmt_prompt.length;
          while(sum-->0)
            put(" ");
          put(cmt_prompt);
        }
      else if(isBracket)
        {
          sum -= ml_prompt.length;
          while(sum-->0)
            put(" ");
          put(ml_prompt);
        }
      else
        put(norm_prompt);
    }

  void addImports(char[][] str...)
    {
      assert(sc !is null);
      sc.registerImport(str);
    }

  // Get the accumulated output since the last call. Includes
  // newlines. Will only work if you have not set an output callback
  // function.
  char[] output()
    {
      assert(!hasCallback);
      char[] res = outBuf.arrayCopy();
      outBuf.length = 0;
      return res;
    }

  // Sets the command prompt (default is "> ")
  void setPrompt(char[] prmt)
    { norm_prompt = prmt; }

  // Sets the multi-line prompt (default is "... ")
  void setMLPrompt(char[] prmt)
    { ml_prompt = prmt; }

  // Set tab size (default 4)
  void setTabSize(int i)
    { tab = i; }

  // Get input. Will sometimes expect multi-line input, for example if
  // case a line contains an open brace or an unterminated block
  // comment. In that case the console will produce another prompt
  // (when you call prompt()), and also return true.
  CR input(char[] str)
    {
      str = str.strip();
      if(str == "") return CR.Empty;

      try return runInput(str);
      catch(MonsterException e)
        {
          putln(e.toString);
          reset();
          return CR.Error;
        }
    }
}

// Statement that handles variable declarations, function calls and
// expression statements in consoles. Since these are gramatically
// similar, we need the type to determine which
class ConsoleStatement : Statement
{
  // Used for variables and expression statements
  Statement client;

  // Used for function calls
  FunctionCallExpr func;

  void parseResolve(ref TokenArray toks, Scope sc, bool allowVar)
    {
      assert(toks.length != 0);

      // Get the first expression
      auto first = Expression.identify(toks);

      // And the type right away
      first.resolve(sc);
      auto type = first.type;
      assert(type !is null);

      // Type? If so, it's a variable declaration
      if((type.isMeta || type.isVar) && allowVar)
        {
          if(type.isMeta) type = type.getBase();

          client = new VarDeclStatement(type);
        }

      // Function?
      else if(type.isFunc)
        func = new FunctionCallExpr(first, toks, true);

      // It's an expression statement
      else
        client = new ExprStatement(first);

      if(client !is null)
        {
          client.parse(toks);
          client.resolve(sc);
        }
      else
        {
          assert(func !is null);
          func.resolve(sc);
        }
    }

 override:
  void parse(ref TokenArray) { assert(0); }
  void resolve(Scope sc) { assert(0); }

  void compile()
    {
      if(client !is null)
        {
          client.compile();
          return;
        }

      assert(func !is null);
      func.evalPop();
    }
}
