/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (vm.d) is part of the Monster script language package.

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

module monster.vm.vm;

import monster.vm.error;
import monster.vm.thread;
import monster.vm.mclass;
import monster.vm.mobject;
import monster.vm.init;
import monster.vm.fstack;

import monster.compiler.tokenizer;
import monster.compiler.linespec;
import monster.compiler.functions;
import monster.compiler.assembler;
import monster.compiler.scopes;

import monster.modules.timer;
import monster.modules.frames;
import monster.modules.vfs;
import monster.options;

import std.stream;
import std.string;
import std.stdio;
import std.utf;
import std.format;
import monster.util.string;

VM vm;

struct VM
{
  // Run a script file in the context of the object obj. If no object
  // is given, an instance of an empty class is used.
  Thread *run(char[] file, MonsterObject *obj = null)
  {
    init();

    Thread *trd;
    auto func = new Function;
    if(obj !is null)
      {
        *func = Function(file, obj.cls);
        trd = func.call(obj);
      }
    else
      {
        *func = Function(file);
        trd = func.call();
      }
    return trd;
  }

  void frame(float time = 0)
  {
    static if(!timer_useClock)
      {
        if(time != 0)
          idleTime.add(time);
      }

    updateFrames(time);

    scheduler.doFrame();
  }

  // Execute a single statement. Context-dependent statements such as
  // 'return' or 'goto' are not allowed, and neither are
  // declarations. Any return value (in case of expressions) is
  // discarded. An optional monster object may be given as context.
  void execute(char[] statm, MonsterObject *mo = null)
  {
    init();

    // Get an empty object if none was specified
    if(mo is null)
      mo = Function.getIntMO();
 
    // Push a dummy function on the stack, tokenize the statement, and
    // parse the various statement types we allow. Assemble it and
    // store the code in the dummy function. The VM handles the rest.
    assert(0, "not done");
  }

  // This doesn't cover the 'console mode', or 'line mode', which is a
  // bit more complicated. (It would search for matching brackets in
  // the token list, print values back out, possibly allow variable
  // declarations, etc.) I think we need a separate Console class to
  // handle this, especially to store temporary values for
  // optimization.

  // Execute an expression and return the value. The template
  // parameter gives the desired type, which must match the type of
  // the expression. An optional object may be given as context.
  T expressionT(T)(char[] expr, MonsterObject *mo = null)
  {
    init();

    // Get an empty object if none was specified
    if(mo is null)
      mo = Function.getIntMO();

    // Push a dummy function on the stack, tokenize and parse the
    // expression. Get the type after resolving, and check it against
    // the template parameter. Execute like for statements, and pop
    // the return value.
    assert(0, "not done");
  }

  // TODO: These have to check for existing classes first. Simply move
  // doLoad over here and fix it up.
  MonsterClass load(char[] nam1, char[] nam2 = "")
  { return doLoad(nam1, nam2, true, true); }

  // Case insensitive with regards to the given class name
  MonsterClass loadCI(char[] nam1, char[] nam2 = "")
  { return doLoad(nam1, nam2, false, true); }

  // Does not fail if the class is not found, just returns null. It
  // will still fail if the class exists and contains errors though.
  MonsterClass loadNoFail(char[] nam1, char[] nam2 = "")
  { return doLoad(nam1, nam2, true, false); }

  // Load a class from a stream. The filename parameter is only used
  // for error messages.
  MonsterClass load(Stream s, char name[] = "", int bom=-1)
  {
    init();

    assert(s !is null, "Cannot load from null stream");
    auto mc = new MonsterClass(-14);
    mc.parse(s, name, bom);
    return mc;
  }

  // Load a class from a token array. The filename parameter is only
  // used for error messages.
  MonsterClass load(ref TokenArray toks, char[] name = "")
  {
    init();

    auto mc = new MonsterClass(-14);
    mc.parse(toks, name);
    return mc;
  }

  // Load a class from a string containing the script. The filename
  // parameter is only used for error messages.
  MonsterClass loadString(char[] str, char[] name="")
  {
    init();

    assert(str != "", "Cannot load empty string");
    auto ms = new MemoryStream(str);
    if(name == "") name = "(string)";

    return load(ms, name);
  }

  static if(enableTrace)
  {
    // Push the given string onto the function stack as an external
    // function.
    void tracef(...)
      {
        char[] s;

        void putc(dchar c)
          {
            std.utf.encode(s, c);
          }

        std.format.doFormat(&putc, _arguments, _argptr);
        trace(s);
      }

    void trace(char[] name) { pushExt(name); }
    void untrace() { popExt(); }
  }
  else
  {
    void tracef(...) {}
    void trace(char[]) {}
    void untrace() {}
  }

  void pushExt(char[] name)
  {
    getFStack().pushExt(name);
  }

  // Pop the last function pushed by pushExt()
  void popExt()
  {
    auto fs = getFStack();
    assert(fs.cur !is null && fs.cur.isExternal,
           "vm.untrace() used on a non-external function stack entry");
    fs.pop();
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

  // Return the current function stack printout
  char[] getTrace()
  { return getFStack().toString(); }

  void addPath(char[] path)
  {
    init();
    addVFS(new FileVFS(path));
  }

  void addVFS(VFS fs) { init(); vfs.add(fs); }
  void addVFSFirst(VFS fs) { init(); vfs.addFirst(fs); }

  void init()
  {
    if(!initHasRun)
      doMonsterInit();
  }

  // This is called from init(), you don't have to call it yourself.
  void doVMInit()
  {
    assert(vfs is null);
    vfs = new ListVFS;

    static if(vmAddCWD) addPath("./");
  }

  ListVFS vfs;

  private:

  // Load file based on file name, class name, or both. The order of
  // the strings doesn't matter, and name2 can be empty. useCase
  // determines if we require a case sensitive match between the given
  // class name and the loaded name. If doThrow is true, we throw an
  // error if the class was not found, otherwise we just return null.
  MonsterClass doLoad(char[] name1, char[] name2, bool useCase, bool doThrow)
  {
    init();

    char[] fname, cname;
    MonsterClass mc;

    if(name1 == "")
      fail("Cannot give empty first parameter to load()");

    if(name1.iEnds(".mn"))
      {
        fname = name1;
        cname = name2;
      }
    else
      {
        fname = name2;
        cname = name1;
      }

    if(cname.iEnds(".mn"))
      fail("load() recieved two filenames: " ~ fname ~ " and " ~ cname);

    // The filename must either be empty, or end with .mn
    if(fname != "" && !fname.iEnds(".mn"))
      fail("Neither " ~ name1 ~ " nor " ~ name2 ~
           " is a valid script filename.");

    // Remember if cname was originally set
    bool cNameSet = (cname != "");

    // Was a filename given?
    if(fname != "")
      {
        // Derive the class name from the file name
        char[] nameTmp = vfs.getBaseName(fname);
        assert(fname.iEnds(".mn"));
        nameTmp = fname[0..$-3];

        if(!cNameSet)
          // No class name given, set it to the derived name
          cname = nameTmp;
        else
          // Both names were given, make sure they match
          if(icmp(nameTmp,cname) != 0)
            fail(format("Class name %s does not match file name %s",
                        cname, fname));
      }
    else
      // No filename given. Derive it from the given class name.
      fname = tolower(cname) ~ ".mn";

    assert(cname != "" && !cname.iEnds(".mn"));
    assert(fname.iEnds(".mn"));

    bool checkFileName()
      {
        if(cname.length == 0)
          return false;

        if(!validFirstIdentChar(cname[0]))
          return false;

        foreach(char c; cname)
          if(!validIdentChar(c)) return false;

        return true;
      }

    if(!checkFileName())
      fail(format("Invalid class name %s (file %s)", cname, fname));

    // At this point, check if the class already exists.
    if(global.ciInList(cname, mc))
      {
        // Match!
        assert(mc !is null);

        // If the class name was given, we must have an exact match.
        if(cNameSet && (cname != mc.name.str))
          fail(format("Searched for %s but could only find case insensitive match %s",
                      cname, mc.name.str));

        // All is good, return the class.
        return mc;
      }

    // No existing class. Search for the script file.
    if(!vfs.has(fname))
      {
        if(doThrow)
          fail("Cannot find script file " ~ fname);
        else return null;
      }

    // Create a temporary file stream and load it
    auto bf = vfs.open(fname);
    auto ef = new EndianStream(bf);
    int bom = ef.readBOM();
    mc = new MonsterClass(-14);
    mc.parse(ef, fname, bom);
    delete bf;

    // After the class is loaded, we can check its real name.

    // If the name matches, we're done.
    if(cname == mc.name.str) return mc;

    // Allow a case insensitive match if useCase is false or the name
    // was not given.
    if((!useCase || !cNameSet) && (icmp(cname, mc.name.str) == 0)) return mc;

    // Oops, name mismatch
    fail(format("%s: Expected class name %s does not match loaded name %s",
                fname, cname, mc.name.str));
    assert(0);
  }
}
