/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (dbg.d) is part of the Monster script language package.

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

module monster.vm.dbg;

import monster.vm.thread;
import std.stream;
import std.string;
import std.cstream;
import monster.vm.fstack;
import monster.options;

/*
  This file is used for runtime debugging, stack tracing, etc.
 */

/*
  Create a trace for your function that automatically pops itself of
  the fstack when it goes out of scope. Usage:

      auto scope _t = new MTrace("funcName");

  This is one of the few places where C++ is actually easier to use...

  An alternative way to do the same thing:

      dbg.trace("funcName");
      scope(exit) dbg.untrace();
*/

scope class MTrace
{
  this(char[] str) { dbg.trace(str); }
  ~this() { dbg.untrace(); }
}

Dbg dbg;

struct Dbg
{
  Stream dbgOut = null;

  // Add indentation for each function stack level (only works if
  // logFStack is true in options.d)
  char[] logLevelString = "| ";

  // Called at startup
  void init()
  {
    static if(defaultLogToStdout)
      dbgOut = dout;
  }

  void log(char[] msg)
  {
    if(dbgOut !is null)
      {
        int logLevel = getFStackLevel();
        int extLevel = getExtLevel();
        int trdIndex = getThreadIndex();

        char[] str = format("(trd=%s,ext=%s,lev=%s)",
                            trdIndex,
                            extLevel,
                            logLevel);
        str = format("%-24s", str);
        dbgOut.writeString(str);

        // If we're logging function stack activity, put in some fancy
        // indentation as well.
        static if(logFStack)
          {
            for(int i;i<logLevel+extLevel;i++)
              dbgOut.writeString(logLevelString);
          }

        dbgOut.writeLine(msg);
      }
  }

  // Stack tracing functions. These might be used internally in the
  // engine, so they should not be disabled.

  // Add an external function to the stack
  void trace(char[] name)
  {
    getFStack().pushExt(name);
  }

  // Pop the last function pushed by trace()
  void untrace()
  {
    auto fs = getFStack();
    assert(fs.cur !is null && fs.cur.isExternal,
           "vm.untrace() used on a non-external function stack entry");
    fs.pop();
  }

  // Return the current function stack printout
  char[] getTrace()
  { return getFStack().toString(); }

  private:

  int getExtLevel()
  {
    return externals.list.length;
  }

  int getFStackLevel()
  {
    if(cthread !is null)
      return cthread.fstack.list.length;
    return 0;
  }

  int getThreadIndex()
  {
    if(cthread !is null)
      return cthread.getIndex();
    else return 0;
  }

  // Get the active function stack, or the externals stack if no
  // thread is active.
  FunctionStack *getFStack()
  {
    if(cthread !is null)
      {
        assert(!cthread.fstack.isEmpty);
        return &cthread.fstack;
      }
    return &externals;
  }
}
