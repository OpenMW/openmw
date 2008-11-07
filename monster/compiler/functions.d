/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
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
import monster.compiler.assembler;
import monster.compiler.bytecode;
import monster.compiler.scopes;
import monster.compiler.variables;
import monster.compiler.tokenizer;
import monster.compiler.linespec;
import monster.compiler.statement;

import monster.vm.mobject;
import monster.vm.idlefunction;
import monster.vm.mclass;
import monster.vm.error;
import monster.vm.fstack;

import std.stdio;

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
  Type type; // Return type
  FuncType ftype; // Function type
  Token name;
  Variable* params[]; // List of parameters
  MonsterClass owner;
  int index; // Unique function identifier within its class

  int paramSize;
  int imprint; // Stack imprint of this function. Equals
               // (type.getSize() - paramSize)

  union
  {
    ubyte[] bcode; // Final compiled code (normal functions)
    dg_callback natFunc_dg; // Various types of native functions
    fn_callback natFunc_fn;
    c_callback natFunc_c;
    IdleFunction idleFunc; // Idle function callback
  }
  LineSpec[] lines; // Line specifications for byte code

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

  // It would be cool to have a template version of call that took and
  // returned the correct parameters, and could even check
  // them. However, doing generic type inference is either very
  // dangerous or would involve complicated checks (think basing an
  // ulong or a a byte to float parameters.) The best thing to do is
  // to handle types at the binding stage, allowing things like
  // bind!(int,float,int)("myfunc", myfunc) - does type checking for
  // you. A c++ version could be much more difficult to handle, and
  // might rely more on manual coding.

  // Used to find the current virtual replacement for this
  // function. The result will depend on the objects real class, and
  // possibly on the object state. Functions might also be overridden
  // explicitly.
  Function *findVirtual(MonsterObject *obj)
  {
    assert(0, "not implemented");
    //return obj.upcast(owner).getVirtual(index);
  }

  // Call the function virtually for the given object
  void vcall(MonsterObject *obj)
  {
    assert(0, "not implemented");
    //obj = obj.upcast(owner);
    //obj.getVirtual(index).call(obj);
  }

  // This is used to call the given function from native code. Note
  // that this is used internally for native functions, but not for
  // any other type. Idle functions can NOT be called directly from
  // native code.
  void call(MonsterObject *obj)
  {
    // Cast the object to the correct type for this function.
    obj = obj.upcast(owner);

    // Push the function on the stack
    fstack.push(this, obj);

    switch(ftype)
      {
      case FuncType.NativeDDel:
        natFunc_dg();
        break;
      case FuncType.NativeDFunc:
        natFunc_fn();
        break;
      case FuncType.NativeCFunc:
        natFunc_c();
        break;
      case FuncType.Normal:
        obj.thread.execute();
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

    // Remove ourselves from the function stack
    fstack.pop();
  }

  // Returns the function name, on the form Class.func()
  char[] toString()
  { return owner.name.str ~ "." ~ name.str ~ "()"; }
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

  // Parse keywords allowed to be used on functions
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
	      if(isNative)
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
	  break;
	}

      // Check that only one of the keywords are used
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
      fn.type.resolve(last);

      // Create a local scope for this function
      sc = new FuncScope(last, fn);

      // Calculate total size of parameters. This value is also used
      // in compile() and by external classes, so we store it.
      fn.paramSize = 0;
      foreach(vd; paramList)
        fn.paramSize += vd.var.type.getSize();

      // Set the owner class.
      fn.owner = sc.getClass();

      // Parameters are given negative numbers according to their
      // position backwards from the stack pointer, the last being
      // -1.
      int pos = -fn.paramSize;

      // Set up the function variable list
      // TODO: Do fancy memory management
      fn.params.length = paramList.length;

      // Add function parameters to scope.
      foreach(i, dec; paramList)
        {
          if(dec.var.type.isArray())
            dec.allowConst = true;

          dec.resolve(sc, pos);

          pos += dec.var.type.getSize();

          fn.params[i] = dec.var;
        }

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
    }

  // Resolve the interior of the function
  void resolveBody()
    {
      // Validate all types (make sure there are no dangling forward
      // references)
      fn.type.validate();
      foreach(p; fn.params)
        p.type.validate();

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
