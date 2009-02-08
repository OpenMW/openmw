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

import std.stdio;
import std.stream;
import std.string;

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
  int index; // Unique function identifier within its class

  int paramSize;

  /*
  int imprint; // Stack imprint of this function. Equals
               // (type.getSize() - paramSize) (not implemented yet)
  */

  // Is this function final? (can not be overridden in child classes)
  bool isFinal;

  // If true, this function can be executed without an object
  bool isStatic;

  // What function we override (if any)
  Function *overrides;

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
    // Check if the file exists
    if(!vm.findFile(file))
      fail("File not found: " ~ file);

    // Create the stream and pass it on
    auto bf = new BufferedFile(file);
    compile(file, bf, mc);
    delete bf;
  }

  void compile(char[] file, Stream str, MonsterClass mc = null)
  {
    // Get the BOM and tokenize the stream
    auto ef = new EndianStream(str);
    int bom = ef.readBOM();
    TokenArray tokens = tokenizeStream(file, ef, bom);
    compile(file, tokens, mc);
    //delete ef;
  }

  void compile(char[] file, ref TokenArray tokens, MonsterClass mc = null)
  {
    assert(name.str == "",
           "Function " ~ name.str ~ " has already been set up");

    // Check if this is a class or a module file first
    if(MonsterClass.canParse(tokens))
      fail("Cannot run " ~ file ~ " - it is a class or module.");

    // Set mc to an empty class if no class is given
    if(mc is null)
      {
        if(int_mc is null)
          {
            assert(int_mo is null);

            int_mc = new MonsterClass(MC.String, int_class);
            int_mo = int_mc.createObject;
          }
        assert(int_mo !is null);

        mc = int_mc;
      }

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

  // Returns the function name, on the form Class.func()
  char[] toString()
  { return owner.name.str ~ "." ~ name.str ~ "()"; }

  private:

  static const char[] int_class = "class _ScriptFile_;";
  static MonsterClass int_mc;
  static MonsterObject *int_mo;
}

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

          isNext(toks, TT.Semicolon);
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
      // Create a Function struct. Will change later.
      fn = new Function;

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
	  // Check that the function declaration ends with a ; rather
	  // than a code block.
	  if(!isNext(toks, TT.Semicolon))
	  {
	    if(fn.isAbstract)
	      fail("Abstract function declaration expected ;", toks);
	    else if(fn.isNative)
	      fail("Native function declaration expected ;", toks);
	    else if(fn.isIdle)
	      fail("Idle function declaration expected ;", toks);
	    else assert(0);
	  }
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
        fail("var not allowed here", fn.type.loc);

      if(fn.type.isReplacer)
        fn.type = fn.type.getBase();

      // Create a local scope for this function
      sc = new FuncScope(last, fn);

      // Calculate total size of parameters. This value is also used
      // in compile() and by external classes, so we store it.
      fn.paramSize = 0;
      foreach(vd; paramList)
        {
          // Resolve the variable first, to make sure we get the rigth
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
    }

  // Resolve the interior of the function
  void resolveBody()
    {
      // Validate all types (make sure there are no dangling forward
      // references)
      fn.type.validate(fn.name.loc);
      foreach(p; fn.params)
        p.type.validate(fn.name.loc);

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
        tasm.exit(fn.paramSize);
      else
	// Functions with return types must have a return statement
	// and should never reach the end of the function. Fail if we
	// do.
	tasm.error(Err.NoReturn);

      // Assemble the finished function
      fn.bcode = tasm.assemble(fn.lines);
    }
}

// Expression representing a function call
class FunctionCallExpr : MemberExpression
{
  Token name;
  ExprArray params;
  Function* fd;

  bool isVararg;

  // Used to simulate a member for imported variables
  DotOperator dotImport;
  bool recurse = true;

  static bool canParse(TokenArray toks)
    {
      return isNext(toks, TT.Identifier) && isNext(toks, TT.LeftParen);
    }

  // Read a parameter list (a,b,...)
  static ExprArray getParams(ref TokenArray toks)
    {
      ExprArray res;
      if(!isNext(toks, TT.LeftParen)) return res;

      Expression exp;

      // No parameters?
      if(isNext(toks, TT.RightParen)) return res;

      // Read the first parameter
      res ~= Expression.identify(toks);

      // Are there more?
      while(isNext(toks, TT.Comma))
        res ~= Expression.identify(toks);

      if(!isNext(toks, TT.RightParen))
	fail("Parameter list expected ')'", toks);

      return res;
    }

  void parse(ref TokenArray toks)
    {
      name = next(toks);
      loc = name.loc;

      params = getParams(toks);
    }

  char[] toString()
    {
      char[] result = name.str ~ "(";
      foreach(b; params)
	result ~= b.toString ~" ";
      return result ~ ")";
    }

  void resolve(Scope sc)
    {
      if(isMember) // Are we called as a member?
	{
          assert(leftScope !is null); 
	  auto l = leftScope.lookup(name);
          fd = l.func;
	  if(!l.isFunc)
            fail(name.str ~ " is not a member function of "
                 ~ leftScope.toString, loc);
	}
      else
	{
          assert(leftScope is null);
	  auto l = sc.lookupImport(name);

          // For imported functions, we have to do some funky magic
          if(l.isImport)
            {
              assert(l.imphold !is null);
              dotImport = new DotOperator(l.imphold, this, loc);
              dotImport.resolve(sc);
              return;
            }

          fd = l.func;
	  if(!l.isFunc)
            fail("Undefined function "~name.str, name.loc);
	}

      type = fd.type;
      assert(type !is null);

      isVararg = fd.isVararg;

      if(isVararg)
        {
          // The vararg parameter can match a variable number of
          // arguments, including zero.
          if(params.length < fd.params.length-1)
            fail(format("%s() expected at least %s parameters, got %s",
                        name.str, fd.params.length-1, params.length),
                 name.loc);
        }
      else
        // Non-vararg functions must match function parameter number
        // exactly
        if(params.length != fd.params.length)
          fail(format("%s() expected %s parameters, got %s",
                      name.str, fd.params.length, params.length),
               name.loc);

      // Check parameter types
      foreach(int i, par; fd.params)
	{
          // Handle varargs below
          if(isVararg && i == fd.params.length-1)
            break;

	  params[i].resolve(sc);
          try par.type.typeCast(params[i]);
	  catch(TypeException)
	    fail(format("%s() expected parameter %s to be type %s, not type %s",
                        name.str, i+1, par.type.toString, params[i].typeString),
                 name.loc);
	}

      // Loop through remaining arguments
      if(isVararg)
        {
          int start = fd.params.length-1;

          assert(fd.params[start].type.isArray);
          Type base = fd.params[start].type.getBase();

          foreach(int i, ref par; params[start..$])
            {
              par.resolve(sc);

              // If the first and last vararg parameter is of the
              // array type itself, then we are sending an actual
              // array. Treat it like a normal parameter.
              if(i == 0 && start == params.length-1 &&
                 par.type == fd.params[start].type)
                {
                  isVararg = false;
                  break;
                }

              // Otherwise, cast the type to the array base type.
              try base.typeCast(par);
              catch(TypeException)
                fail(format("Cannot convert %s of type %s to %s", par.toString,
                            par.typeString, base.toString), par.loc);
            }
        }
    }

  // Used in cases where the parameters need to be evaluated
  // separately from the function call. This is done by DotOperator,
  // in cases like obj.func(expr); Here expr is evaluated first, then
  // obj, and then finally the far function call. This is because the
  // far function call needs to read 'obj' off the top of the stack.
  void evalParams()
    {
      assert(pdone == false);

      foreach(i, ex; params)
        {
          ex.eval();

          // Convert 'const' parameters to actual constant references
          if(i < fd.params.length-1) // Skip the last parameter (in
                                     // case of vararg functions)
            if(fd.params[i].isConst)
              {
                assert(fd.params[i].type.isArray);
                tasm.makeArrayConst();
              }
        }

      if(isVararg)
        {
          // Compute the length of the vararg array.
          int len = params.length - fd.params.length + 1;

          // If it contains no elements, push a null array reference
          // (0 is always null).
          if(len == 0) tasm.push(0);
          else
            // Converte the pushed values to an array index
            tasm.popToArray(len, params[$-1].type.getSize());
        }

      // Handle the last parameter after everything has been
      // pushed. This will work for both normal and vararg parameters.
      if(fd.params.length != 0 && fd.params[$-1].isConst)
        {
          assert(fd.params[$-1].type.isArray);
          tasm.makeArrayConst();
        }

      pdone = true;
    }

  bool pdone;

  void evalAsm()
    {
      if(dotImport !is null && recurse)
        {
          recurse = false;
          dotImport.evalAsm();
          recurse = true;
          return;
        }

      if(!pdone) evalParams();
      setLine();
      assert(fd.owner !is null);

      if(fd.isIdle)
        tasm.callIdle(fd.index, fd.owner.getTreeIndex(), isMember);
      else
        tasm.callFunc(fd.index, fd.owner.getTreeIndex(), isMember);

      // Reset pdone incase the expression is evaluated again
      pdone = false;
    }
}
