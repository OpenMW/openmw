/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
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

import std.string;
import std.stdio;
import std.uni;
import std.c.string;

import monster.compiler.bytecode;
import monster.compiler.linespec;
import monster.compiler.states;
import monster.compiler.functions;
import monster.compiler.scopes;

import monster.vm.mclass;
import monster.vm.mobject;
import monster.vm.codestream;
import monster.vm.stack;
import monster.vm.scheduler;
import monster.vm.idlefunction;
import monster.vm.arrays;
import monster.vm.iterators;
import monster.vm.error;
import monster.vm.fstack;

// Used for array copy below. It handles overlapping data for us.
extern(C) void* memmove(void *dest, void *src, size_t n);

extern(C) double floor(double d);

// This represents an execution 'thread' in the system. Each object
// has its own thread. The thread contains a link to the object and
// the class, along with some other data.
struct CodeThread
{
  /*******************************************************
   *                                                     *
   *     Public variables                                *
   *                                                     *
   *******************************************************/

  // The object that "owns" this thread. This can only point to the
  // top-most object in the linked parent object chain.
  MonsterObject* topObj;

  // Pointer to our current scheduling point. If null, we are not
  // currently sceduled. Only applies to state code, not scheduled
  // function calls.
  CallNode scheduleNode;


  /*******************************************************
   *                                                     *
   *     Public functions                                *
   *                                                     *
   *******************************************************/

  void initialize(MonsterObject* top)
  {
    topObj = top;

    // Initialize other variables
    state = null; // Start in the empty state
    scheduleNode = null;
    isActive = false;
    stateChange = false;
  }

  State* getState() { return state; }

  // Call state code for this object. 'pos' gives the byte position
  // within the bytecode. It is called when a new state is entered, or
  // when an idle funtion returns. The state must already be set with
  // setState
  void callState(int pos)
  {
    assert(state !is null, "attempted to call the empty state");
    assert(!isActive,
           "callState cannot be called when object is already active");
    assert(fstack.isEmpty,
	   "ctate code can only run at the bottom of the function stack");

    // Set a bool to indicate that we are now actively running state
    // code.
    isActive = true;

    // Set up the code stack
    fstack.push(state, topObj.upcast(state.sc.getClass()));

    // Set the position
    fstack.cur.code.jump(pos);

    // Run the code
    execute();

    // We are no longer active
    isActive = false;

    assert(stack.getPos == 0,
           format("Stack not returned to zero after state code, __STACK__=",
                  stack.getPos));

    fstack.pop();
  }

  /* Set state. Invoked by the statement "state = statename;". This
     function can be called in several situations, with various
     results:

     + setState called with current state, no label
       -> no action is performed

     + setState called with another state
     + setState called with current state + a label
       -> state is changed normally

     If a state change takes place directly in state code, the code is
     aborted immediately. If it takes place in a function called from
     state code, then code flow is allowed to return normally back to
     the state code level, but is aborted immediately once it reaches
     state code.

     State changes outside state code will always unschedule any
     previously scheduled code (such as idle functions, or previous
     calls to setState.)
   */
  void setState(State *st, StateLabel *label)
  {
    // If no label is specified and we are already in this state, then
    // do nothing.
    if(st is state && label is null)
      return;

    // Does the state actually change?
    if(st !is state)
      {
        // If so, we must handle state functions and other magic here.
      }

    // Set the state
    state = st;

    // If we are already scheduled (if an idle function has scheduled
    // us, or if setState has been called multiple times),
    // unschedule. This will automatically cancel any scheduled idle
    // functions and call their abort() functions.
    if(scheduleNode)
      scheduleNode.cancel();

    // If we are jumping to anything but the empty state, we might
    // have to schedule some code.
    if(st !is null)
      {
        // Check that this state is valid
        assert(st.sc.getClass().parentOf(topObj), "state '" ~ st.name.str ~
               "' is not part of class " ~ topObj.cls.getName());

        if(label is null)
          // findLabel will return null if the label is not found.
          // TODO: The begin label should probably be cached within
          // State.
          label = st.findLabel("begin");

        // Reschedule the new state for the next frame, if a label is
        // specified. We have to cast to find the right object first
        // though.
        auto mo = topObj.upcast(st.sc.getClass());

        if(label !is null)
          scheduler.scheduleState(mo, label.offs);
      }

    assert(isActive || !stateChange,
           "stateChange was set outside active code");

    // If we are running from state code, signal it that we must now
    // abort execution when we reach the state level.
    stateChange = isActive;
  }

  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/
  private:

  bool isActive; // Set to true whenever we are running from state
		 // code. If we are inside the state itself, this will
		 // be true and 'next' will be 1.
  bool stateChange; // Set to true when a state change is in
		    // progress. Only used when state is changed from
		    // within a function in active code.
  State *state;  // Current state, null is the empty state.

  /*******************************************************
   *                                                     *
   *     Private helper functions                        *
   *                                                     *
   *******************************************************/

  void fail(char[] msg)
  {
    int line = -1;
    if(fstack.cur !is null)
      line = fstack.cur.code.getLine();

    .fail(msg, topObj.cls.name.loc.fname, line);
  }

  // Index version of setState - called from bytecode
  void setState(int st, int label, int cls)
  {
    if(st == -1)
      {
        assert(label == -1);
        setState(null, null);
        return;
      }

    auto mo = topObj.upcastIndex(cls);

    auto pair = mo.cls.findState(st, label);
    setState(pair.state, pair.label);

    assert(pair.state.index == st);
    assert(pair.state.sc.getClass().getIndex == cls);
  }

  void callIdle()
  {
    assert(isActive && fstack.isStateCode,
           "Byte code attempted to call an idle function outside of state code.");

    CodeStream *code = &fstack.cur.code;

    // Get the correct object
    MonsterObject *mo = topObj.upcastIndex(code.getInt());

    // And the function
    Function *fn = mo.cls.findFunction(code.getInt());
    assert(fn !is null && fn.isIdle);

    // The IdleFunction object bound to this function is stored in
    // fn.idleFunc
    if(fn.idleFunc is null)
      fail("Called unimplemented idle function '" ~ fn.name.str ~ "'");

    // Tell the scheduler that an idle function was called. It
    // will reschedule us as needed.
    scheduler.callIdle(mo, fn, fstack.cur.code.getPos);
  }

  // Pops a pointer off the stack. Null pointers will throw an
  // exception.
  int *popPtr(MonsterObject *obj)
  {
    PT type;
    int index;
    decodePtr(stack.popInt(), type, index);

    // Null pointer?
    if(type == PT.Null)
      fail("Cannot access value, null pointer");
  
    // Local variable?
    if(type == PT.Stack)
      return stack.getFrameInt(index);

    // Variable in this object
    if(type == PT.DataOffs)
      return obj.getDataInt(index);

    // This object, but another (parent) class
    if(type == PT.DataOffsCls)
      {
        // We have to pop the class index of the stack as well
        return obj.upcastIndex(stack.popInt()).getDataInt(index);
      }

    // Far pointer, with offset. Both the class index and the object
    // reference is on the stack.
    if(type == PT.FarDataOffs)
      {
        int clsIndex = stack.popInt();

	// Get the object reference from the stack
	MonsterObject *tmp = stack.popObject();

        // Cast the object to the correct class
        tmp = tmp.upcastIndex(clsIndex);

	// Return the correct pointer
	return tmp.getDataInt(index);
      }

    // Array pointer
    if(type == PT.ArrayIndex)
      {
        assert(index==0);
        // Array indices are on the stack, not in the opcode.
        index = stack.popInt();
        ArrayRef *arf = stack.popArray();
        assert(!arf.isNull);
        if(arf.isConst)
          fail("Cannot assign to constant array");
        index *= arf.elemSize;
        if(index < 0 || index >= arf.iarr.length)
          fail("Array index " ~ .toString(index/arf.elemSize) ~
               " out of bounds (array length " ~ .toString(arf.length) ~ ")");
        return &arf.iarr[index];
      }

    fail("Unable to handle pointer type " ~ toString(cast(int)type));
  }

  bool shouldExitState()
  {
    if(fstack.isStateCode && stateChange)
      {
        assert(isActive);

        // The state was changed while in state code. Abort the code.
        stateChange = false;

        // There might be dangling stack values
        stack.reset();
        return true;
      }
    return false;
  }


  /*******************************************************
   *                                                     *
   *     execute() - main VM function                    *
   *                                                     *
   *******************************************************/
  public:

  // Execute instructions in the current function stack entry. This is
  // the main workhorse of the VM, the "byte-code CPU". The function
  // is called (possibly recursively) whenever a byte-code function is
  // called, and returns when the function exits.
  void execute()
  {
    // The maximum amount of instructions we execute before assuming
    // an infinite loop.
    const long limit = 10000000;

    assert(fstack.cur !is null,
	   "CodeThread.execute called but there is no code on the function stack.");

    // Get some values from the function stack
    CodeStream *code = &fstack.cur.code;
    MonsterObject *obj = fstack.cur.obj;
    MonsterClass cls = obj.cls;

    // Only an object belonging to this thread can be passed to
    // execute() on the function stack.
    assert(cls.parentOf(topObj));

    // Reduce or remove as many of these as possible
    int *ptr;
    long *lptr;
    float *fptr;
    double *dptr;
    int[] iarr;
    ArrayRef *arf;
    int val, val2;
    long lval;

    // Disable this for now. It should be a per-function option, perhaps,
    // or at least a compile time option.
    //for(long i=0;i<limit;i++)
    for(;;)
      {
	ubyte opCode = code.getCmd();

        //writefln("stack=", stack.getPos);
        //writefln("exec(%s): %s", code.getLine, bcToString[opCode]);

	switch(opCode)
	  {

	  case BC.Exit:
            // Exit execute(). The function stack cleanup is handled
            // by Our Glorious Caller.
            return;

	  case BC.Call:
            {
              // Get the correct object. TODO: Later we might make an
              // optimized version for when this is not needed. That
              // goes for all function call types.
              MonsterObject *mo = obj.upcastIndex(code.getInt());

              // Call the function
              mo.cls.findFunction(code.getInt()).call(mo);

              if(shouldExitState()) return;
              break;
            }

          case BC.CallFar:
            {
              // Get object to work on
              MonsterObject *mo = stack.popObject().upcastIndex(code.getInt());
              mo.cls.findFunction(code.getInt()).call(mo);

              // Exit state code if the state was changed
              if(shouldExitState()) return;
              break;
            }

          case BC.CallIdle:
            // Initiate the idle function.
            assert(isActive && fstack.isStateCode,
                   "idle call encountered outside state code.");
            callIdle();
            return;

	  case BC.Return:
	    // Remove the given number of bytes from the stack, and
	    // exit the function.
	    stack.pop(code.getInt());
            return;

	  case BC.ReturnVal:
	    stack.pop(code.getInt(), 1);
            return;

	  case BC.ReturnValN:
	    val = code.getInt(); // Get the value first, since order
				 // of evaluation is important.
	    stack.pop(val, code.getInt());
            return;

	  case BC.State:
            val = code.getInt(); // State index
            val2 = code.getInt(); // Label index
	    // Get the class index and let setState handle everything
	    setState(val, val2, code.getInt());
            if(shouldExitState()) return;
	    break;

          case BC.Halt:
            // A halt statement was encountered, and we should stop
            // execution. This is only allowed in state code.
            assert(isActive && fstack.isStateCode,
                   "halt command encountered outside state code.");
            return;

	  case BC.New:
	    // Create a new object. Look up the class index in the
	    // global class table, and create an object from it.
	    stack.pushObject(global.getClass(cast(CIndex)code.getInt())
                             .createObject());
	    break;

          case BC.Clone:
            stack.pushObject(stack.popObject().clone());
            break;

	  case BC.Jump:
	    code.jump(code.getInt);
	    break;

	  case BC.JumpZ:
	    val = code.getInt;
	    if(stack.popInt() == 0)
	      code.jump(val);
	    break;

	  case BC.JumpNZ:
	    val = code.getInt;
	    if(stack.popInt() != 0)
	      code.jump(val);
	    break;

	  case BC.PushData:
	    stack.pushInt(code.getInt());
	    break;

	  case BC.PushLocal:
	    stack.pushInt(*stack.getFrameInt(code.getInt()));
	    break;

	  case BC.PushClassVar:
	    stack.pushInt(*obj.getDataInt(code.getInt()));
	    break;

          case BC.PushParentVar:
            {
              // Get object to work on.
              MonsterObject *mo = obj.upcastIndex(code.getInt());
              assert(mo !is obj, "should use PushClassVar");
              stack.pushInt(*mo.getDataInt(code.getInt()));
            }
            break;

	  case BC.PushFarClassVar:
            {
              // Get object to work on
              MonsterObject *mo = stack.popObject().upcastIndex(code.getInt());
              stack.pushInt(*mo.getDataInt(code.getInt()));
            }
	    break;

	  case BC.PushFarClassMulti:
            {
              val = code.getInt(); // Variable size
              // Get object to work on
              MonsterObject *mo = stack.popObject().upcastIndex(code.getInt());
              val2 = code.getInt(); // Data segment offset
              stack.pushInts(mo.getDataArray(val2,val));
            }
	    break;

	  case BC.PushThis:
	    // Push the index of this object.
	    stack.pushObject(obj);
	    break;

	  case BC.Pop: stack.popInt(); break;

	  case BC.PopN: stack.pop(code.get()); break;

	  case BC.Dup: stack.pushInt(*stack.getInt(0)); break;

	  case BC.StoreRet:
	    // Get the pointer off the stack, and convert it to a real
	    // pointer.
	    ptr = popPtr(obj);
	    // Read the value and store it, but leave it in the stack
	    *ptr = *stack.getInt(0);
	    break;

	  case BC.StoreRet8:
            ptr = popPtr(obj);
            *(cast(long*)ptr) = *stack.getLong(1);
            break;

            // Int / uint operations
	  case BC.IAdd:
	    ptr = stack.getInt(1);
	    *ptr += stack.popInt;
	    break;

	  case BC.ISub:
	    ptr = stack.getInt(1);
	    *ptr -= stack.popInt;
	    break;

	  case BC.IMul:
	    ptr = stack.getInt(1);
	    *ptr *= stack.popInt;
	    break;

	  case BC.IDiv:
	    ptr = stack.getInt(1);
	    val = stack.popInt;
	    if(val)
	      {
		*ptr /= val;
		break;
	      }
	    fail("Integer division by zero");

	  case BC.UDiv:
	    ptr = stack.getInt(1);
	    val = stack.popInt;
	    if(val)
	      {
		*(cast(uint*)ptr) /= cast(uint)val;
		break;
	      }
	    fail("Integer division by zero");

	  case BC.IDivRem:
	    ptr = stack.getInt(1);
            val = stack.popInt;
            if(val)
              {
                *ptr %= val;
                break;
              }
	    fail("Integer division by zero");

	  case BC.UDivRem:
	    ptr = stack.getInt(1);
            val = stack.popInt;
            if(val)
              {
                *(cast(uint*)ptr) %= cast(uint)val;
                break;
              }
	    fail("Integer division by zero");

	  case BC.INeg:
	    ptr = stack.getInt(0);
	    *ptr = -*ptr;
	    break;


            // Float operations
	  case BC.FAdd:
	    fptr = stack.getFloat(1);
	    *fptr += stack.popFloat;
	    break;

	  case BC.FSub:
	    fptr = stack.getFloat(1);
	    *fptr -= stack.popFloat;
	    break;

	  case BC.FMul:
	    fptr = stack.getFloat(1);
	    *fptr *= stack.popFloat;
	    break;

          case BC.FDiv:
            fptr = stack.getFloat(1);
            *fptr /= stack.popFloat;
            break;

          case BC.FIDiv:
            fptr = stack.getFloat(1);
            *fptr = floor(*fptr / stack.popFloat);
            break;

            // Calculate a generalized reminder for floating point
            // numbers
          case BC.FDivRem:
            {
              fptr = stack.getFloat(1);
              float fval = stack.popFloat;
              *fptr -= fval * floor(*fptr/fval);
            }
            break;

	  case BC.FNeg:
	    fptr = stack.getFloat(0);
	    *fptr = -*fptr;
	    break;

            // Long / ulong operations
	  case BC.LAdd:
	    lptr = stack.getLong(3);
	    *lptr += stack.popLong;
	    break;

	  case BC.LSub:
	    lptr = stack.getLong(3);
	    *lptr -= stack.popLong;
	    break;

	  case BC.LMul:
	    lptr = stack.getLong(3);
	    *lptr *= stack.popLong;
	    break;

	  case BC.LDiv:
	    lptr = stack.getLong(3);
	    lval = stack.popLong;
	    if(lval)
	      {
		*lptr /= lval;
		break;
	      }
	    fail("Long division by zero");

	  case BC.ULDiv:
	    lptr = stack.getLong(3);
	    lval = stack.popLong;
	    if(lval)
	      {
		*(cast(ulong*)lptr) /= cast(ulong)lval;
		break;
	      }
	    fail("Long division by zero");

	  case BC.LDivRem:
	    lptr = stack.getLong(3);
            lval = stack.popLong;
            if(lval)
              {
                *lptr %= lval;
                break;
              }
	    fail("Long division by zero");

	  case BC.ULDivRem:
	    lptr = stack.getLong(3);
            lval = stack.popLong;
            if(lval)
              {
                *(cast(ulong*)lptr) %= cast(ulong)lval;
                break;
              }
	    fail("Long division by zero");

	  case BC.LNeg:
	    lptr = stack.getLong(1);
	    *lptr = -*lptr;
	    break;

            // Double operations
	  case BC.DAdd:
	    dptr = stack.getDouble(3);
	    *dptr += stack.popDouble;
	    break;

	  case BC.DSub:
	    dptr = stack.getDouble(3);
	    *dptr -= stack.popDouble;
	    break;

	  case BC.DMul:
	    dptr = stack.getDouble(3);
	    *dptr *= stack.popDouble;
	    break;

          case BC.DDiv:
            dptr = stack.getDouble(3);
            *dptr /= stack.popDouble;
            break;

          case BC.DIDiv:
            dptr = stack.getDouble(3);
            *dptr = floor(*dptr / stack.popDouble);
            break;

            // Calculate a generalized reminder for floating point
            // numbers
          case BC.DDivRem:
            {
              dptr = stack.getDouble(3);
              double fval = stack.popDouble;
              *dptr -= fval * floor(*dptr/fval);
            }
            break;

	  case BC.DNeg:
	    dptr = stack.getDouble(1);
	    *dptr = -*dptr;
	    break;

	  case BC.IsEqual:
            stack.pushBool(stack.popInt == stack.popInt);
	    break;

          case BC.IsEqualMulti:
            val = code.getInt(); // Get the variable size (in ints)
            assert(val > 1);
            stack.pushBool(stack.popInts(val) ==
                           stack.popInts(val));
            break;

	  case BC.IsCaseEqual:
	    if(toUniLower(stack.popChar) == toUniLower(stack.popChar)) stack.pushInt(1);
	    else stack.pushInt(0);
	    break;

	  case BC.CmpArray:
            stack.pushBool(stack.popArray().iarr == stack.popArray().iarr);
            break;

	  case BC.ICmpStr:
            stack.pushBool(isUniCaseEqual(stack.popString(),
                                          stack.popString()));
            break;

	  case BC.PreInc:
	    ptr = popPtr(obj);
	    stack.pushInt(++(*ptr));
	    break;

	  case BC.PreDec:
	    ptr = popPtr(obj);
	    stack.pushInt(--(*ptr));
	    break;

	  case BC.PostInc:
	    ptr = popPtr(obj);
	    stack.pushInt((*ptr)++);
	    break;

	  case BC.PostDec:
	    ptr = popPtr(obj);
	    stack.pushInt((*ptr)--);
	    break;

	  case BC.PreInc8:
	    lptr = cast(long*)popPtr(obj);
	    stack.pushLong(++(*lptr));
	    break;

	  case BC.PreDec8:
	    lptr = cast(long*)popPtr(obj);
	    stack.pushLong(--(*lptr));
	    break;

	  case BC.PostInc8:
	    lptr = cast(long*)popPtr(obj);
	    stack.pushLong((*lptr)++);
	    break;

	  case BC.PostDec8:
	    lptr = cast(long*)popPtr(obj);
	    stack.pushLong((*lptr)--);
	    break;

	  case BC.Not:
	    ptr = stack.getInt(0);
	    if(*ptr == 0) *ptr = 1;
	    else *ptr = 0;
	    break;

	  case BC.ILess:
	    val = stack.popInt;
	    if(stack.popInt < val) stack.pushInt(1);
	    else stack.pushInt(0);
	    break;

	  case BC.ULess:
	    val = stack.popInt;
	    if(stack.popUint < cast(uint)val) stack.pushInt(1);
	    else stack.pushInt(0);
	    break;

	  case BC.LLess:
	    lval = stack.popLong;
	    if(stack.popLong < lval) stack.pushInt(1);
	    else stack.pushInt(0);
	    break;

	  case BC.ULLess:
	    lval = stack.popLong;
	    if(stack.popUlong < cast(ulong)lval) stack.pushInt(1);
	    else stack.pushInt(0);
	    break;

	  case BC.FLess:
            {
              float fval = stack.popFloat;
              if(stack.popFloat < fval) stack.pushInt(1);
              else stack.pushInt(0);
              break;
            }

	  case BC.DLess:
            {
              double fval = stack.popDouble;
              if(stack.popDouble < fval) stack.pushInt(1);
              else stack.pushInt(0);
              break;
            }

          case BC.CastI2L:
            if(*stack.getInt(0) < 0) stack.pushInt(-1);
            else stack.pushInt(0);
            //stack.pushLong(stack.popInt());
            break;

            // Castint to float
          case BC.CastI2F:
            ptr = stack.getInt(0);
            fptr = cast(float*) ptr;
            *fptr = *ptr;
            break;

          case BC.CastU2F:
            ptr = stack.getInt(0);
            fptr = cast(float*) ptr;
            *fptr = *(cast(uint*)ptr);
            break;

          case BC.CastL2F:
            stack.pushFloat(stack.popLong);
            break;

          case BC.CastUL2F:
            stack.pushFloat(stack.popUlong);
            break;

          case BC.CastD2F:
            stack.pushFloat(stack.popDouble);
            break;

            // Castint to double
          case BC.CastI2D:
            stack.pushDouble(stack.popInt);
            break;

          case BC.CastU2D:
            stack.pushDouble(stack.popUint);
            break;

          case BC.CastL2D:
            stack.pushDouble(stack.popLong);
            break;

          case BC.CastUL2D:
            stack.pushDouble(stack.popUlong);
            break;

          case BC.CastF2D:
            stack.pushDouble(stack.popFloat);
            break;

          case BC.CastI2S:
            {
              val = code.get();
              char[] res;
              if(val == 1) res = .toString(stack.popInt);
              else if(val == 2) res = .toString(stack.popUint);
              else if(val == 3) res = .toString(stack.popLong);
              else if(val == 4) res = .toString(stack.popUlong);
              else assert(0);
              stack.pushArray(res);
            }
            break;

          case BC.CastF2S:
            {
              val = code.get();
              char[] res;
              if(val == 1) res = .toString(stack.popFloat);
              else if(val == 2) res = .toString(stack.popDouble);
              else assert(0);
              stack.pushArray(res);
            }
            break;

          case BC.CastB2S:
            stack.pushArray(.toString(stack.popBool));
            break;

          case BC.CastO2S:
            {
              MIndex idx = stack.popMIndex();
              if(idx != 0)
                stack.pushArray(format("%s#%s", getMObject(idx).cls.getName,
                                       cast(int)idx));
              else
                stack.pushCArray("(null object)");
            }
            break;

          case BC.Upcast:
            // TODO: If classes ever get more than one index, it might
            // be more sensible to use the global class index here.
            stack.pushObject(stack.popObject().upcastIndex(code.getInt()));
            break;

          case BC.FetchElem:
            // This is not very optimized
            val = stack.popInt(); // Index
            arf = stack.popArray(); // Get the array
            if(val < 0 || val >= arf.length)
              fail("Array index " ~ .toString(val) ~ " out of bounds (array length is "
                   ~ .toString(arf.length) ~ ")");
            val *= arf.elemSize;
            for(int i = 0; i<arf.elemSize; i++)
              stack.pushInt(arf.iarr[val+i]);
            break;

          case BC.GetArrLen:
            stack.pushInt(stack.popArray().length);
            break;

          case BC.MakeArray:
            val = code.getInt(); // The data segment offset
            val2 = code.getInt(); // Element size
            // Get the raw length (array length * elem size) and
            // create the new array from data.
            stack.pushArray(obj.getDataArray(val, code.getInt()), val2);
            break;

          case BC.PopToArray:
            val = code.getInt; // Raw length
            val2 = code.getInt; // Element size
            ptr = stack.getInt(val-1); // getInt checks if the range is valid
            iarr = ptr[0..val].dup; // Make sure to copy the data
            stack.pop(val); // Remove it from the stack
            stack.pushArray(iarr, val2);
            break;

          case BC.NewArray:
            val = code.getInt(); // Array nesting (dimension) level
            val2 = code.getInt(); // Element size
            iarr = code.getIntArray(val2); // Initial value
            // Create the array
            createMultiDimArray(val, iarr);
            break;

          case BC.CopyArray:
            arf = stack.popArray(); // Destination
            if(arf.isConst)
              fail("Cannot copy to constant array");
            iarr = stack.popArray().iarr; // Source
            if(arf.iarr.length != iarr.length)
              fail(format("Array length mismatch (%s != %s)",
                          arf.iarr.length, iarr.length));
            // Push back the destination
            stack.pushArray(arf);

            // Use memmove, since it will handle overlapping data
            memmove(arf.iarr.ptr, iarr.ptr, iarr.length*4);
            break;

          case BC.DupArray:
            arf = stack.popArray();
            if(!arf.isNull)
              // Only create a new array index if it is non-null. Dup
              // of null is just null.
              stack.pushArray(arf.iarr.dup, arf.elemSize);
            else
              stack.pushArray(arf);
            break;

          case BC.MakeConstArray:
            ptr = stack.getInt(0);
            arf = arrays.getRef(cast(AIndex)*ptr);
            if(!arf.isNull && !arf.isConst)
              *ptr = cast(int) arrays.createConst(arf.iarr, arf.elemSize).getIndex;
            break;

          case BC.IsConstArray:
            stack.pushBool(stack.popArray().isConst);
            break;

          case BC.Slice:
            {
              int i2 = stack.popInt(); // Last index
              int i1 = stack.popInt();
              arf = stack.popArray();
              if(arf.isNull)
                {
                  if(i1 != 0 || i2 != 0)
                    fail(format("Slice indices [%s..%s] out of range (array is empty)",
                                i1, i2));
                  // Push the null array back
                  stack.pushArray(arf);
                  break;
                }

              // Get the int indices
              val = i1*arf.elemSize;
              val2 = i2*arf.elemSize;

              if(val < 0 || val2 < 0 || val > arf.iarr.length || val2 > arf.iarr.length ||
                 val > val2)
                fail(format("Slice indices [%s..%s] out of range (array length is %s)",
                            i1, i2, arf.length));

              // Slices of constant arrays are also constant
              if(arf.isConst)
                arf = arrays.createConst(arf.iarr[val..val2], arf.elemSize);
              else
                arf = arrays.create(arf.iarr[val..val2], arf.elemSize);

              stack.pushArray(arf);
              break;
            }

          case BC.FillArray:
            arf = stack.popArray();
            if(arf.isConst)
              fail("Cannot fill a constant array");
            val = code.getInt(); // Element size
            assert(val == arf.elemSize || arf.isNull);
            iarr = stack.popInts(val); // Pop the value

            // Fill the array
            assert(arf.iarr.length % val == 0);
            for(int i=0; i<arf.iarr.length; i+=val)
              arf.iarr[i..i+val] = iarr[];

            stack.pushArray(arf);
            break;

          case BC.CatArray:
            arf = stack.popArray(); // Right array
            if(arf.isNull)
              {
                // right is empty, just copy the left
                arf = stack.popArray();
                if(arf.isNull) stack.pushArray(arf);
                else stack.pushArray(arf.iarr.dup, arf.elemSize);
              }
            else
              {
                iarr = stack.popArray().iarr ~ arf.iarr; // Create new array
                stack.pushArray(iarr, arf.elemSize); // Push it back
              }
            break;

          case BC.CatLeft:
            val = code.getInt(); // Element size
            arf = stack.popArray(); // Array
            iarr = stack.popInts(val); // Element
            assert(arf.elemSize == val || arf.isNull);

            iarr ~= arf.iarr; // Put in new element and make a new
                              // array
            stack.pushArray(iarr, val);
            break;

          case BC.CatRight:
            val = code.getInt(); // Element size
            iarr = stack.popInts(val); // Element
            arf = stack.popArray(); // Array
            assert(arf.elemSize == val || arf.isNull);

            iarr = arf.iarr ~ iarr;
            stack.pushArray(iarr, val);
            break;

          case BC.ReverseArray:
            arf = stack.getArray(0);
            if(arf.isConst)
              fail("Cannot reverse constant array");
            val2 = arf.elemSize;
            if(val2 == 1) arf.iarr.reverse;
            else if(val2 == 2) (cast(long[])arf.iarr).reverse;
            else if(val2 > 2)
              {
                assert(arf.iarr.length % val2 == 0);
                val = arf.length / 2; // Half the number of elements (rounded down)
                val *= val2; // Multiplied back up to number of ints
                for(int i=0; i<val; i+=val2)
                  swap(arf.iarr[i..i+val2], arf.iarr[$-i-val2..$-i]);
              }
            else assert(arf.isNull);
            break;

          case BC.CreateArrayIter:
            val = code.get(); // reverse order?
            val2 = code.get(); // reference variable?
            ptr = stack.getInt(0); // gets the address of the array index
            stack.pushBool(iterators.firstArray(val!=0, val2!=0, ptr));
            break;

          case BC.CreateClassIter:
            val = code.getInt(); // Class number

            // Make room on the stack for the iterator index
            stack.pushInt(0);

            ptr = stack.getInt(1); // Get address of the two indices
                                   // on the stack
            stack.pushBool(iterators.firstClass(global.getClass(cast(CIndex)val),
                                                ptr[0..2]));
            break;

          case BC.IterNext:
            stack.pushBool(iterators.next(cast(IIndex)*stack.getInt(0)));
            break;

          case BC.IterBreak:
            iterators.stop(cast(IIndex)*stack.getInt(0));
            break;

          case BC.IterUpdate:
	    val = code.getInt(); // Get stack index of iterator reference
            if(val < 0) fail("Invalid argument to IterUpdate");
	    val = *stack.getFrameInt(val); // Iterator index
            iterators.update(cast(IIndex)val); // Do the update
            break;

	  case BC.GetStack:
	    stack.pushInt(stack.getPos);
	    break;

          case BC.MultiByte:
            fail("Multibyte instructions not implemented");
            return;

	  case BC.Error:
	    fail(format("%s.\n", errorString(code.get())));
	    return;

	  default:
            if(opCode >= bcToString.length)
              fail(format("Invalid command opcode %s", opCode));
            else
              fail(format("Unimplemented opcode '%s' (%s)",
                          bcToString[opCode], opCode));
	  }
      }
    fail(format("Execution unterminated after %s instructions.", limit,
		" Possibly an infinite loop, aborting."));
  }
}

// Helper function for reversing arrays. Swaps the contents of two
// arrays.
void swap(int[] a, int[] b)
{
  const BUF = 32;

  assert(a.length == b.length);
  int[BUF] buf;
  uint len = a.length;

  while(len >= BUF)
    {
      buf[] = a[0..BUF];
      a[0..BUF] = b[0..BUF];
      b[0..BUF] = buf[];

      a = a[BUF..$];
      b = b[BUF..$];
      len -= BUF;
    }

  if(len)
    {
      buf[0..len] = a[];
      a[] = b[];
      b[] = buf[0..len];
    }
}
