/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (functions.d) is part of the Monster script language
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

module monster.compiler.functions;

enum FuncType
  {
    Normal,      // Normal function, defined in script code
    Native,      // Unassigned native function
    NativeDFunc, // Native D function
    NativeDDel,  // Native D delegate
    NativeCFunc, // Native C function
    Abstract,    // Abstract, does not have a body
    Idle,        // Idle function, can only be called in state code
  }

import monster.compiler.types;
import monster.compiler.operators;
import monster.compiler.assembler;
import monster.compiler.bytecode;
import monster.compiler.scopes;
import monster.compiler.expression;
import monster.compiler.variables;
import monster.compiler.tokenizer;
import monster.compiler.linespec;
import monster.compiler.statement;

import monster.vm.mobject;
import monster.vm.idlefunction;
import monster.vm.mclass;
import monster.vm.error;
import monster.vm.thread;
import monster.vm.stack;
import monster.vm.vm;

import monster.util.growarray;

import std.stdio;
import std.stream;
import std.string;

version(Tango) import tango.core.Traits;
else import std.traits;

// One problem with these split compiler / vm classes is that we
// likely end up with data (or at least pointers) we don't need, and a
// messy interface. The problem with splitting is that we duplicate
// code and definitions. One solution is to let the VM class be a
// separate class (should be in vm/), but containing all we need in
// the VM (like the code, list of parameters, etc.) The point of this
// class (which we can rename FunctionCompiler, and leave in this
// file) is to create, build and nurture the Function it creates. The
// Function can be a struct, really, but I'll look into that. Flipping
// function structs off a region and pointing to them is easy and
// efficient, but creating classes isn't much worse. It depends if we
// need to inherit from them, really.

// Used for native functions
alias void delegate() dg_callback;
typedef void function() fn_callback;
typedef extern(C) void function() c_callback;

struct Function
{
  MonsterClass owner; // Must be the first entry

  LineSpec[] lines; // Line specifications for byte code
  union
  {
    ubyte[] bcode; // Final compiled code (normal functions)
    dg_callback natFunc_dg; // Various types of native functions
    fn_callback natFunc_fn;
    c_callback natFunc_c;
    IdleFunction idleFunc; // Idle function callback
  }
  Token name;

  Type type; // Return type
  FuncType ftype; // Function type
  Variable* params[]; // List of parameters
  int[][] defaults; // Default parameter values (if specified, null otherwise)
  int index; // Unique function identifier within its class

  // Register this function in the global function list. This can only
  // be done once, but it's required before functions can be
  // referenced by function pointers in script code. Don't call this
  // if you don't know what you're doing.
  void register()
  {
    assert(!hasGIndex(), "Don't call register() more than once.");

    gIndex = functionList.length();
    functionList ~= this;
  }

  int getGIndex()
  {
    assert(hasGIndex(), "This function doesn't have a global index");
    return gIndex;
  }

  bool hasGIndex() { return gIndex != -1; }

  static Function *fromIndex(int index)
  {
    if(index < 0)
      fail("Null function reference encountered");

    if(index > functionList.length)
      fail("Invalid function index encountered");

    return functionList[index];
  }

  int paramSize;

  // Is this function final? (can not be overridden in child classes)
  bool isFinal;

  // If true, this function can be executed without an object
  bool isStatic;

  // What function we override (if any)
  Function *overrides;

  // Find the virtual replacement for this function in the context of
  // the object mo.
  Function *findVirtual(MonsterObject *mo)
  {
    assert(mo !is null);
    return findVirtual(mo.cls);
  }

  // Find virtual replacement in the context of class cls.
  Function *findVirtual(MonsterClass cls)
  {
    assert(cls.childOf(owner));
    return cls.findVirtualFunc(owner.getTreeIndex(), index);
  }

  bool isNormal() { return ftype == FuncType.Normal; }
  bool isNative()
  {
    return
      ftype == FuncType.Native || ftype == FuncType.NativeDFunc ||
      ftype == FuncType.NativeDDel || ftype == FuncType.NativeCFunc; 
  }
  bool isAbstract() { return ftype == FuncType.Abstract; }
  bool isIdle() { return ftype == FuncType.Idle; }

  // True if the last parameter is a vararg parameter, meaning that
  // this is a function that takes a variable number of arguments.
  bool isVararg() { return params.length && params[$-1].isVararg; }

  // Bind the given function template parameter to this Function. The
  // function is assumed to have compatible parameter and return
  // types, and the values are pushed and popped of the Monster stack
  // automatically.
  void bindT(alias func)()
  {
    assert(isNative, "cannot bind to non-native function " ~ name.str);

    version(Tango)
      {
        alias ParameterTypleOf!(func) P;
        alias ReturnTypeOf!(func) R;
      }
    else
      {
        alias ParameterTypeTuple!(func) P;
        alias ReturnType!(func) R;
      }

    assert(P.length == params.length, format("function %s has %s parameters, but binding function has %s", name.str, params.length, P.length));

    // Check parameter types
    foreach(int i, p; P)
      assert(params[i].type.isDType(typeid(p)), format(
        "binding %s: type mismatch in parameter %s, %s != %s",
        name.str, i, params[i].type, typeid(p)));

    // Check the return type
    static if(is(R == void))
      {
        assert(type.isVoid, format("binding %s: expected to return type %s",
                                   name.str, type));
      }
    else
      {
        assert(!type.isVoid, format(
          "binding %s: function does not have return value %s",
          name.str, typeid(R)));

        assert(type.isDType(typeid(R)), format(
          "binding %s: mismatch in return type, %s != %s",
          name.str, type, typeid(R)));
      }

    // This is the actual function that is bound, and called each time
    // the native function is invoked.
    void delegate() dg =
      {
        P parr;

        foreach_reverse(int i, PT; P)
        {
          parr[i] = stack.popType!(PT)();
        }

        static if(is(R == void))
        {
          func(parr);
        }
        else
        {
          R r = func(parr);
          stack.pushType!(R)(r);
        }
      };

    // Store the function
    ftype = FuncType.NativeDDel;
    natFunc_dg = dg;
  }

  template callT(T)
  {
    T callT(A...)(MonsterObject *mo, A a)
      {
        // Check parameter types
        foreach(int i, p; A)
          assert(params[i].type.isDType(typeid(p)), format(
            "calling %s: type mismatch in parameter %s, %s != %s",
            name.str, i, params[i].type, typeid(p)));

        // Check the return type
        static if(is(T == void))
          {
            assert(type.isVoid, format("calling %s: expected to return type %s",
                                       name.str, type));
          }
        else
          {
            assert(!type.isVoid, format(
              "calling %s: function does not have return value %s",
              name.str, typeid(T)));

            assert(type.isDType(typeid(T)), format(
              "calling %s: mismatch in return type, %s != %s",
              name.str, type, typeid(T)));
          }

        // Push all the values
        foreach(i, AT; A)
          stack.pushType!(AT)(a[i]);

        call(mo);

        static if(!is(T == void))
          // Get the return value
          return stack.popType!(T)();
      }
  }

  // This is used to call the given function from native code. Note
  // that this is used internally for native functions, but not for
  // any other type. Idle functions can NOT be called directly from
  // native code. Returns the thread, which is never null but might be
  // dead.
  Thread *call(MonsterObject *obj)
  {
    // Make sure there's a thread to use
    if(cthread is null)
      {
        // Get a new thread and put it in the foreground
        auto tr = Thread.getNew();
        tr.foreground();
        assert(tr is cthread);
      }

    assert(cthread !is null);
    assert(!cthread.isDead);

    bool wasEmpty = cthread.fstack.isEmpty;

    // Push the function on the stack
    cthread.fstack.push(this, obj);

    switch(ftype)
      {
      case FuncType.NativeDDel:
        natFunc_dg();
        goto pop;
      case FuncType.NativeDFunc:
        natFunc_fn();
        goto pop;
      case FuncType.NativeCFunc:
        natFunc_c();
      pop:
        // Remove ourselves from the function stack
        cthread.fstack.pop();
        break;
      case FuncType.Normal:
        cthread.execute();
        assert(!cthread.shouldExit,
               "shouldExit should only be set for state threads");
        // Execute will pop itself if necessary
        break;
      case FuncType.Native:
        fail("Called unimplemented native function " ~ toString);
      case FuncType.Idle:
        fail("Cannot call idle function " ~ toString ~ " from native code");
      case FuncType.Abstract:
        fail("Called unimplemented abstract function " ~ toString);
      default:
        assert(0, "unknown FuncType for " ~ toString);
      }

    // If we started at the bottom of the function stack, put the
    // thread in the background now. This will automatically delete
    // the thread if it's not used.
    assert(cthread !is null);
    auto ct = cthread;
    if(wasEmpty)
      {
        if(cthread.fstack.isEmpty && stack.getPos != 0)
          {
            assert(cthread.isTransient);

            // We have to do some trickery to retain the stack in
            // cases where the function exits completely.
            cthread = null; // This will prevent kill() from clearing
                            // the stack.
            ct.kill();
          }
        else
          cthread.background();
      }
    return ct;
  }

  // Call without an object. TODO: Only allowed for functions compiled
  // without a class using compile() below, but in the future it will
  // be allowed for static function.
  Thread* call()
  {
    assert(owner is int_mc);
    assert(owner !is null);
    assert(int_mo !is null);
    return call(int_mo);
  }

  // This allows you to compile a function file by writing fn =
  // Function("filename").
  static Function opCall(char[] file, MonsterClass mc = null)
  {
    Function fn;
    fn.compile(file, mc);
    return fn;
  }

  // Compile the function script 'file' in the context of the class
  // 'mc'. If no class is given, use an empty internal class.
  void compile(char[] file, MonsterClass mc = null)
  {
    vm.init();

    // Check if the file exists
    if(!vm.vfs.has(file))
      fail("File not found: " ~ file);

    // Create the stream and pass it on
    auto bf = vm.vfs.open(file);
    compile(file, bf, mc);
    delete bf;
  }

  void compile(char[] file, Stream str, MonsterClass mc = null)
  {
    vm.init();

    // Get the BOM and tokenize the stream
    auto ef = new EndianStream(str);
    int bom = ef.readBOM();
    TokenArray tokens = tokenizeStream(file, ef, bom);
    compile(file, tokens, mc);
    //delete ef;
  }

  void compile(char[] file, ref TokenArray tokens, MonsterClass mc = null)
  {
    vm.init();

    assert(name.str == "",
           "Function " ~ name.str ~ " has already been set up");

    // Check if this is a class or a module file first
    if(MonsterClass.canParse(tokens))
      fail("Cannot run " ~ file ~ " - it is a class or module.");

    // Set mc to the empty class if no class is given
    if(mc is null)
      mc = getIntMC();

    auto fd = new FuncDeclaration;
    // Parse and comile the function
    fd.parseFile(tokens, this);
    name.str = file;
    name.loc.fname = file;
    fd.resolve(mc.sc);
    fd.resolveBody();
    fd.compile();
    assert(fd.fn == this);
    delete fd;
  }

  static MonsterClass getIntMC()
  {
    if(int_mc is null)
      {
        assert(int_mo is null);
        int_mc = vm.loadString(int_class);
        int_mo = int_mc.createObject;
      }
    assert(int_mo !is null);
    return int_mc;
  }

  static MonsterObject *getIntMO()
  {
    getIntMC();
    return int_mo;
  }

  // Returns the function name, on the form Class.func()
  char[] toString()
  { return owner.name.str ~ "." ~ name.str ~ "()"; }

  private:

  // Global unique function index
  int gIndex = -1;

  // Empty class / object used internally
  static const char[] int_class = "class _ScriptFile_;";
  static MonsterClass int_mc;
  static MonsterObject *int_mo;
}

// A specialized function declaration that handles class constructors
class Constructor : FuncDeclaration
{
  static bool canParse(TokenArray toks)
    { return toks.isNext(TT.New); }

  void parse(ref TokenArray toks)
    {
      // Create a Function struct.
      fn = new Function;

      // Default function type is normal
      fn.ftype = FuncType.Normal;

      // No return value
      fn.type = BasicType.getVoid;

      // Parse
      toks.reqNext(TT.New, fn.name);
      loc = fn.name.loc;
      code = new CodeBlock;
      code.parse(toks);
    }

  char[] toString()
    {
      char[] res = "Constructor:\n";
      assert(code !is null);
      res ~= code.toString();
      return res;
    }

  // Resolve the constructor
  void resolve(Scope last)
    {
      assert(fn.type !is null);

      // Create a local scope for this function
      sc = new FuncScope(last, fn);

      // Set the owner class
      auto cls = sc.getClass();
      fn.owner = cls;

      // Make sure we're assigned to the class
      assert(cls.scptConst is this);

      // Resolve the function body
      assert(code !is null);
      code.resolve(sc);
    }
}

// Global list of functions.
GrowArray!(Function*) functionList;

// Responsible for parsing, analysing and compiling functions.
class FuncDeclaration : Statement
{
  CodeBlock code;
  VarDeclaration[] paramList;
  FuncScope sc; // Scope used internally in the function body

  // The persistant function definition. This data will be passed to
  // the VM when the compiler is done working.
  Function *fn;

  // Is the 'override' keyword present
  bool isOverride;

  // Is this a stand-alone script file (not really needed)
  bool isFile;

  // Parse keywords allowed to be used on functions. This (and its
  // borthers elsewhere) is definitely ripe for some pruning /
  // refactoring.
  private void parseKeywords(ref TokenArray toks)
    {
      Floc loc;

      // Get the old state
      bool isNative = fn.isNative;
      bool isAbstract = fn.isAbstract;
      bool isIdle = fn.isIdle;

      while(1)
	{
	  if(isNext(toks, TT.Native, loc))
	    {
	      if(isNative)
		fail("Multiple token 'native' in function declaration",
		     loc);
	      isNative = true;
	      continue;
	    }
	  if(isNext(toks, TT.Abstract, loc))
	    {
	      if(isAbstract)
		fail("Multiple token 'abstract' in function declaration",
		     loc);
	      isAbstract = true;
	      continue;
	    }
	  if(isNext(toks, TT.Idle, loc))
	    {
	      if(isIdle)
		fail("Multiple token 'idle' in function declaration",
		     loc);
	      isIdle = true;
	      continue;
	    }
	  if(isNext(toks, TT.Override, loc))
	    {
	      if(isOverride)
		fail("Multiple token 'override' in function declaration",
		     loc);
	      isOverride = true;
	      continue;
	    }
	  if(isNext(toks, TT.Final, loc))
	    {
	      if(fn.isFinal)
		fail("Multiple token 'final' in function declaration",
		     loc);
	      fn.isFinal = true;
	      continue;
	    }
	  break;
	}

      // Check that only one of the type keywords are used
      if( (isAbstract && isNative) ||
          (isAbstract && isIdle) ||
          (isNative && isIdle) )
        fail("Only one of the keywords native, idle, abstract can be used on one function", loc);

      // Set the new state
      if(isNative) fn.ftype = FuncType.Native;
      else if(isAbstract) fn.ftype = FuncType.Abstract;
      else if(isIdle) fn.ftype = FuncType.Idle;
      else assert(fn.isNormal);
    }

  void parseFile(ref TokenArray toks, Function *fnc)
    {
      isFile = true;
      fn = fnc;
      fn.ftype = FuncType.Normal;

      // Is there an explicit function declaration?
      if(isNext(toks, TT.Function))
        {
          TokenArray temp = toks;

          reqNext(temp, TT.Identifier);

          // Is this a function without type?
          if(isFuncDec(toks))
            // If so, set the type to void
            fn.type = BasicType.getVoid;
          else
            // Otherwise, parse it
            fn.type = Type.identify(toks);

          // In any case, parse the rest of the declaration
          parseParams(toks);

          reqSep(toks);
        }
      else
        {
          // No type, no parameters
          fn.type = BasicType.getVoid;
          fn.name.str = "script-file";
        }

      code = new CodeBlock(false, true);
      code.parse(toks);
    }

  void parse(ref TokenArray toks)
    {
      // Create a Function struct.
      fn = new Function;

      // Register a global index for all class functions
      fn.register();
      assert(fn.getGIndex() != -1);

      // Default function type is normal
      fn.ftype = FuncType.Normal;

      // Parse keyword list
      parseKeywords(toks);

      // Is this a function without type?
      if(isFuncDec(toks))
	// If so, set the type to void
	fn.type = BasicType.getVoid;
      else
	// Otherwise, parse it
	fn.type = Type.identify(toks);

      // Parse any other keywords
      parseKeywords(toks);

      parseParams(toks);

      if(fn.isAbstract || fn.isNative || fn.isIdle)
	{
          reqSep(toks);
	}
      else
	{
	  code = new CodeBlock;
	  code.parse(toks);
	}
    }

  // Parse function name and parameters
  void parseParams(ref TokenArray toks)
    {
      fn.name = next(toks);
      loc = fn.name.loc;
      if(fn.name.type != TT.Identifier)
	fail("Token '" ~ fn.name.str ~ "' cannot be used as a function name",
	     loc);

      if(!isNext(toks, TT.LeftParen))
	fail("Function expected parameter list", toks);

      // Parameters?
      if(!isNext(toks, TT.RightParen))
	{
	  auto vd = new VarDeclaration();
	  vd.parse(toks);
	  paramList ~= vd;

	  // Other parameters
	  while(isNext(toks, TT.Comma))
	    {
	      vd = new VarDeclaration();
	      vd.parse(toks);
	      paramList ~= vd;
	    }

          // Vararg-parameter?
          if(isNext(toks, TT.DDDot))
            paramList[$-1].var.isVararg = true;

	  if(!isNext(toks, TT.RightParen))
	    fail("Expected end of parameter list", toks);
	}
    }

  // Can the given tokens be parsed as the main function declaration?
  static bool isFuncDec(TokenArray toks)
    {
      return isNext(toks, TT.Identifier) && isNext(toks, TT.LeftParen);
    }

  static bool canParse(TokenArray toks)
    {
      // Is the next token an allowed keyword?
      bool isKeyword(ref TokenArray toks)
	{
	  return
	    isNext(toks, TT.Native) ||
	    isNext(toks, TT.Abstract) ||
	    isNext(toks, TT.Override) ||
	    isNext(toks, TT.Final) ||
	    isNext(toks, TT.Idle);
	}

      // Remove keywords
      while(isKeyword(toks)) {}

      // We allow the declaration to have no type (which implies type
      // void)
      if(isFuncDec(toks)) return true;

      // The next token(s) must be the type
      if(!Type.canParseRem(toks)) return false;

      // There might be more keywords
      while(isKeyword(toks)) {}

      // Finally we must have the function declaration at the end
      return isFuncDec(toks);
    }

  char[] toString()
    {
      char[] res = "Function declaration: ";
      assert(fn.type !is null);

      res ~= fn.type.toString();

      res ~= " " ~ fn.name.str ~ "(";
      if(paramList.length)
	{
	  if(paramList.length > 1)
	    foreach(par; paramList[0..paramList.length-1])
	      res ~= par.toString ~ ", ";

	  res ~= paramList[$-1].toString;
	}

      res ~= ")\n";
      if(code !is null) res ~= code.toString();
      return res;
    }

  // Resolve the function definition (return type and parameter
  // types). The rest is handed by resolveBody()
  void resolve(Scope last)
    {
      assert(fn.type !is null);

      fn.type.resolve(last);

      if(fn.type.isVar)
        fail("var not allowed as function return type", fn.type.loc);

      if(fn.type.isReplacer)
        fn.type = fn.type.getBase();

      // Create a local scope for this function
      sc = new FuncScope(last, fn);

      // Calculate total size of parameters. This value is also used
      // in compile() and by external classes, so we store it.
      fn.paramSize = 0;
      foreach(vd; paramList)
        {
          // Resolve the variable first, to make sure we get the right
          // size
          vd.resolveParam(sc);
          assert(!vd.var.type.isReplacer);

          fn.paramSize += vd.var.type.getSize();
        }

      // Set the owner class
      fn.owner = sc.getClass();

      // Parameters are given negative numbers according to their
      // position backwards from the stack pointer, the last being
      // -1.
      int pos = -fn.paramSize;

      // Set up the function variable list
      // TODO: Do fancy memory management
      fn.params.length = paramList.length;

      // Set up function parameter numbers and insert them into the
      // list.
      foreach(i, dec; paramList)
        {
          assert(pos < 0);
          dec.setNumber(pos);
          pos += dec.var.type.getSize();
          fn.params[i] = dec.var;
        }
      assert(pos == 0);

      // Vararg functions must have the last parameter as an array.
      if(fn.isVararg)
        {
          assert(paramList.length > 0);
          auto dc = paramList[$-1];
          if(!dc.var.type.isArray)
            fail("Vararg argument must be an array type, not " ~
                 dc.var.type.toString, dc.var.name.loc);
        }

      assert(pos == 0, "Variable positions didn't add up");

      // Do we override a function?
      if(fn.overrides)
        {
          Function *o = fn.overrides;
          assert(fn.owner !is null);
          assert(o.owner !is null,
                 "overrided function must be resolved before us");

          if(fn.owner is o.owner)
            fail(format("Function %s is already declared on line %s",
                        fn.name.str, o.name.loc.line), fn.name.loc);

          // Check that the function we're overriding isn't final
          if(o.isFinal)
            fail("Cannot override final function " ~ o.toString, fn.name.loc);

          // Check that signatures match
          if(o.type != fn.type)
            fail(format("Cannot override %s with different return type (%s -> %s)",
                        fn.name.str, o.type, fn.type), fn.name.loc);

          bool parFail = false;
          if(o.params.length != fn.params.length) parFail = true;
          else
            foreach(i,p; fn.params)
              if(p.type != o.params[i].type ||
                 p.isVararg != o.params[i].isVararg)
                {
                  parFail = true;
                  break;
                }

          if(parFail)
            fail(format("Cannot override %s, parameter types do not match",
                        o.toString), fn.name.loc);

          if(o.isIdle && !fn.isIdle)
            fail("Cannot override idle function " ~ o.name.str ~
                 " with a non-idle function", fn.name.loc);
          if(!o.isIdle && fn.isIdle)
            fail("Cannot override normal function " ~ o.name.str ~
                 " with an idle function", fn.name.loc);
        }
      else
        {
          // No overriding. Make sure the 'override' flag isn't set.
          if(isOverride)
            fail("function " ~ fn.name.str ~
                 " doesn't override anything", fn.name.loc);
        }

      // Get the values of parameters which have default values
      // assigned
      fn.defaults.length = paramList.length;
      foreach(i, dec; paramList)
        {
          if(dec.init !is null)
            {
              if(fn.isVararg)
                fail("Vararg functions cannot have default parameter values", fn.name.loc);

              // Get the value and store it. Fails if the expression
              // is not computable at compile time.
              fn.defaults[i] = dec.getCTimeValue();
              assert(fn.defaults[i].length > 0);
            }
        }

    }

  // Resolve the interior of the function
  void resolveBody()
    {
      // Validate all types (make sure there are no dangling forward
      // references)
      fn.type.validate(fn.name.loc);
      foreach(p; fn.params)
        p.type.validate(fn.name.loc);

      // Resolve the function body
      if(code !is null)
	code.resolve(sc);
    }

  void compile()
    {
      if(fn.isAbstract || fn.isNative || fn.isIdle)
	{
          // No body to compile
	  return;
	}

      tasm.newFunc();
      code.compile();

      tasm.setLine(code.endLine.line);

      if(fn.type.isVoid)
        // Remove parameters from the stack at the end of the function
        tasm.exit(fn.paramSize,0,0);
      else
	// Functions with return types must have a return statement,
	// so we should never reach the end of the function. Fail if
	// we do.
	tasm.error(Err.NoReturn);

      // Assemble the finished function
      fn.bcode = tasm.assemble(fn.lines);
    }
}

struct NamedParam
{
  Token name;
  Expression value;
}

// Expression representing a function call
class FunctionCallExpr : Expression
{
  Expression fname;   // Function name or pointer value
  ExprArray params;   // Normal (non-named) parameters
  NamedParam[] named; // Named parameters

  ExprArray coverage; // Expressions sorted in the same order as the
                      // function parameter list. Null expressions
                      // means we must use the default value. Never
                      // used for vararg functions.

  // These are used to get information about the function and
  // parameters. If this is a function reference call, fd is null and
  // only frt is set.
  Function* fd;
  FuncRefType frt;

  bool isCast;        // If true, this is an explicit typecast, not a
                      // function call.

  bool isVararg;

  bool fResolved;     // True if fname is already resolved

  // Read a function parameter list (a,b,v1=c,v2=d,...). The function
  // expects that you have already removed the initial left paren '('
  // token.
  static void getParams(ref TokenArray toks,
                        out ExprArray parms,
                        out NamedParam[] named)
    {
      parms = null;
      named = null;

      // No parameters?
      if(isNext(toks, TT.RightParen)) return;

      // Read the comma-separated list of parameters
      do
        {
          if(toks.length < 2)
            fail("Unexpected end of stream");

          // Named paramter?
          if(toks[1].type == TT.Equals)
            {
              NamedParam np;

              reqNext(toks, TT.Identifier, np.name);
              reqNext(toks, TT.Equals);
              np.value = Expression.identify(toks);

              named ~= np;
            }
          else
            {
              // Normal parameter
              if(named.length)
                fail("Cannot use non-named parameters after a named one",
                     toks[0].loc);

              parms ~= Expression.identify(toks);
            }
        }
      while(isNext(toks, TT.Comma))

      if(!isNext(toks, TT.RightParen))
	fail("Parameter list expected ')'", toks);
    }

  // Special version of getParams that allow 'console-mode' grammar.
  static void getParamsConsole(ref TokenArray toks,
                               out ExprArray parms,
                               out NamedParam[] named)
    {
      parms = null;
      named = null;

      // The param list is terminated by a semicolon or a newline
      while(!isSep(toks))
        {
          if(toks.length == 0)
            fail("Unexpected end of stream");

          // Named paramter?
          if(toks.length >= 3 && toks[1].type == TT.Equals)
            {
              NamedParam np;

              reqNext(toks, TT.Identifier, np.name);
              reqNext(toks, TT.Equals);
              np.value = Expression.identify(toks);

              named ~= np;
            }
          else
            {
              // Normal parameter
              if(named.length)
                fail("Cannot use non-named parameters after a named one",
                     toks[0].loc);

              parms ~= Expression.identify(toks);
            }

          // Allow optional commas between parameters
          isNext(toks, TT.Comma);
        }
    }

  this(Expression func, ref TokenArray toks, bool console=false)
    {
      assert(func !is null);
      fname = func;
      loc = fname.loc;

      // Parse the parameter list. The 'console' parameter determines
      // whether we're using normal grammar rules:
      //     func(param1,param2)
      // or console rules
      //     func param1 param2
      if(console)
        getParamsConsole(toks, params, named);
      else
        getParams(toks, params, named);
      fResolved = console;
    }

  void parse(ref TokenArray toks) { assert(0); }

  char[] toString()
    {
      char[] result = fname.toString ~ "(";
      foreach(b; params)
	result ~= b.toString ~", ";
      foreach(b; named)
        result ~= b.name.str ~ "=" ~ b.value.toString ~ ", ";
      return result ~ ")";
    }

  char[] name() { assert(fname !is null); return fname.toString(); }

  void resolve(Scope sc)
    {
      // Resolve the function lookup first
      if(!fResolved)
        fname.resolve(sc);
      assert(fname.type !is null);

      // Is the 'function' really a type name?
      if(fname.type.isMeta)
        {
          // If so, it's a type cast! Get the type we're casting to.
          type = fname.type.getBase();
          assert(type !is null);

          // Only one (non-named) parameter is allowed
          if(params.length != 1 || named.length != 0)
            fail("Invalid parameter list to type cast", loc);

          isCast = true;

          // Resolve the expression we're converting
          params[0].resolve(sc);

          // Cast it
          type.typeCastExplicit(params[0]);

          return;
        }

      // TODO: Do typecasting here. That will take care of polysemous
      // types later as well.
      if(!fname.type.isIntFunc && !fname.type.isFuncRef)
        fail(format("Expression '%s' of type %s is not a function",
                    fname, fname.typeString), loc);

      if(fname.type.isFuncRef)
        {
          // Set frt to the reference type
          frt = cast(FuncRefType)fname.type;
          assert(frt !is null);
          fd = null;
        }
      else if(fname.type.isIntFunc)
        {
          // Get the function from the type
          auto ft = cast(IntFuncType)fname.type;
          assert(ft !is null);
          fd = ft.func;

          // Create a temporary reference type
          frt = new FuncRefType(ft);
        }

      isVararg = frt.isVararg;
      type = frt.retType;
      assert(type !is null);

      if(fd is null && named.length != 0)
        fail("Cannot use named parameters when calling function references",
             loc);

      // Get the best parameter name possible from the available
      // information
      char[] getParName(int i)
        {
          char[] dst = "parameter ";
          if(fd !is null)
            dst ~= fd.params[i].name.str;
          else
            dst ~= .toString(i);
          return dst;
        }

      if(isVararg)
        {
          if(named.length)
            fail("Cannot give named parameters to vararg functions", loc);

          // The vararg parameter can match a variable number of
          // arguments, including zero.
          if(params.length < frt.params.length-1)
            fail(format("%s() expected at least %s parameters, got %s",
                        name, frt.params.length-1, params.length),
                 loc);

          // Check parameter types except for the vararg parameter
          foreach(int i, par; frt.params[0..$-1])
            {
              params[i].resolve(sc);

              // The info about each parameter depends on whether this
              // is a direct function call or a func reference call.
              par.typeCast(params[i], getParName(i));
            }

          // Loop through remaining arguments
          int start = frt.params.length-1;

          assert(frt.params[start].isArray);
          Type base = frt.params[start].getBase();

          foreach(int i, ref par; params[start..$])
            {
              par.resolve(sc);

              // If the first and only vararg parameter is of the
              // array type itself, then we are sending an actual
              // array. Treat it like a normal parameter.
              if(i == 0 && start == params.length-1 &&
                 par.type == frt.params[start])
                {
                  isVararg = false;
                  coverage = params;
                  break;
                }

              // Otherwise, cast the type to the array base type.
              base.typeCast(par, "vararg array base type");
            }
          return;
        }

      // Non-vararg case. Non-vararg functions must cover at least all
      // the non-optional function parameters.

      int parNum = frt.params.length;

      // When calling a function reference, we can't use named
      // parameters, and we don't know the function's default
      // values. Because of this, all parameters must be present.
      if(fd is null && params.length != parNum)
        fail(format("Function reference %s (of type '%s') expected %s arguments, got %s",
                    name, frt, parNum, params.length), fname.loc);

      // Sanity check on the parameter number for normal function
      // calls
      if(params.length > parNum)
        fail(format("Too many parameters to function %s(): expected %s, got %s",
                    name, parNum, params.length), fname.loc);

      // Make the coverage list of all the parameters.
      coverage = new Expression[parNum];

      // Mark all the parameters which are present. Start with the
      // sequential (non-named) paramteres.
      foreach(i,p; params)
        {
          assert(coverage[i] is null);
          assert(p !is null);
          coverage[i] = p;
        }

      if(fd !is null)
        {
          // This part (named and optional parameters) does not apply
          // when calling function references

          assert(fd !is null);

          // Add named parameters to the list
          foreach(p; named)
            {
              // Look up the named parameter
              int index = -1;
              foreach(i, fp; fd.params)
                if(fp.name.str == p.name.str)
                  {
                    index = i;
                    break;
                  }
              if(index == -1)
                fail(format("Function %s() has no parameter named %s",
                            name, p.name.str),
                     p.name.loc);

              assert(index<parNum);

              // Check that the parameter isn't already set
              if(coverage[index] !is null)
                fail("Parameter " ~ p.name.str ~ " set multiple times ",
                     p.name.loc);

              // Finally, set the parameter
              coverage[index] = p.value;
              assert(coverage[index] !is null);
            }

          // Check that all non-optional parameters are present
          assert(fd.defaults.length == coverage.length);
          foreach(i, cv; coverage)
            if(cv is null && fd.defaults[i].length == 0)
              fail(format("Non-optional parameter %s is missing in call to %s()",
                          fd.params[i].name.str, name),
                   loc);
        }

      // Check parameter types
      foreach(int i, ref cov; coverage)
	{
          // Skip missing parameters
          if(cov is null)
            {
              assert(fd !is null);
              continue;
            }

          auto par = frt.params[i];

	  cov.resolve(sc);
          par.typeCast(cov, getParName(i));
	}
    }

  // Evaluate the parameters
  private void evalParams()
    {
      // Handle the vararg case separately
      if(isVararg)
        {
          assert(coverage is null);

          // Eval the parameters
          foreach(i, ex; params)
            {
              ex.eval();

              // The rest only applies to non-vararg parameters
              if(i >= frt.params.length-1)
                continue;

              // Convert 'const' parameters to actual constant references
              if(frt.isConst[i])
                {
                  assert(frt.params[i].isArray);
                  tasm.makeArrayConst();
                }
            }

          // Compute the length of the vararg array.
          int len = params.length - frt.params.length + 1;

          // If it contains no elements, push a null array reference
          // (0 is always null).
          if(len == 0) tasm.push(0);
          else
            {
              // Convert the pushed values to an array index
              tasm.popToArray(len, params[$-1].type.getSize());

              // Convert the vararg array to 'const' if needed
              if(frt.isConst[$-1])
                {
                  assert(frt.params[$-1].isArray);
                  tasm.makeArrayConst();
                }
            }
          return;
        }

      // Non-vararg case
      assert(!isVararg);
      assert(coverage.length == frt.params.length);
      foreach(i, ex; coverage)
        {
          if(ex !is null)
            {
              assert(ex.type !is null);
              assert(frt.params[i] !is null);
              assert(ex.type == frt.params[i]);

              ex.eval();
            }
          else
            {
              // resolve() should already have caught this
              assert(fd !is null,
                     "cannot use default parameters with reference calls");

              // No param specified, use default value
              assert(fd.defaults[i].length ==
                     fd.params[i].type.getSize);
              assert(fd.params[i].type.getSize > 0);
              tasm.pushArray(fd.defaults[i]);
            }

          // Convert 'const' parameters to actual constant references
          if(frt.isConst[i])
            {
              assert(frt.params[i].isArray);
              tasm.makeArrayConst();
            }
        }
    }

  void evalAsm()
    {
      if(isCast)
        {
          // This is a type cast, not a function call.

          // Just evaluate the expression. CastExpression takes care
          // of everything automatically.
          assert(params.length == 1);
          assert(params[0] !is null);
          params[0].eval();

          return;
        }
      assert(frt !is null);

      // Push parameters first
      evalParams();

      // Then push the function expression or reference.
      fname.eval();
      setLine();

      bool isFar;

      // Total stack imprint of the function call
      int imprint;

      if(fd !is null)
        {
          // Direct function call
          assert(fname.type.isIntFunc);
          auto ft = cast(IntFuncType)fname.type;
          assert(ft !is null);

          assert(fd.owner !is null);

          // Calculate the stack imprint
          imprint = fd.type.getSize() - fd.paramSize;

          // Push the function index. For far function calls, we have
          // in effect pushed a function reference (object+function)
          tasm.push(fd.getGIndex());

          isFar = ft.isMember;
        }
      else
        {
          // Function reference call
          assert(fname.type.isFuncRef);
          isFar = true;

          // Let the type calculate the stack imprint
          auto frt = cast(FuncRefType)fname.type;
          assert(frt !is null);

          imprint = frt.getImprint();
        }

      tasm.call(isFar, imprint);
    }
}
