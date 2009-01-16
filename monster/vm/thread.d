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

module monster.vm.thread;

import std.string;
import std.stdio;
import std.uni;
import std.c.string;

import monster.util.freelist;

import monster.compiler.bytecode;
import monster.compiler.linespec;
import monster.compiler.states;
import monster.compiler.functions;
import monster.compiler.scopes;
import monster.compiler.types;

import monster.vm.mclass;
import monster.vm.mobject;
import monster.vm.codestream;
import monster.vm.stack;
import monster.vm.idlefunction;
import monster.vm.arrays;
import monster.vm.iterators;
import monster.vm.error;
import monster.vm.fstack;

// Used for array copy below. It handles overlapping data for us.
extern(C) void* memmove(void *dest, void *src, size_t n);

extern(C) double floor(double d);

import monster.util.list;
alias _lstNode!(Thread) _tmp1;
alias __FreeNode!(Thread) _tmp2;
alias FreeList!(Thread) NodeList;

// Current thread
Thread *cthread;

// This represents an execution 'thread' in the system. Each object
// has its own thread. The thread contains a link to the object and
// the class, along with some other data.
struct Thread
{
  /*******************************************************
   *                                                     *
   *     Public variables                                *
   *                                                     *
   *******************************************************/

  // This has been copied from ScheduleStruct, which is now merged
  // with Thread. We'll sort it out later.

  // Some generic variables that idle functions can use to store
  // temporary data off the stack.
  SharedType idleData;

  // The contents of idleObj's extra data for the idle's owner class.
  SharedType extraData;

  // Set to true whenever we are running from state code. If we are
  // inside the state itself, this will be true and 'next' will be 1.
  bool isActive; 

  // Set to true when a state change is in progress. Only used when
  // state is changed from within a function in active code.
  bool stateChange;

  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/

  private:
  // Temporarily needed since we need a state and an object to push on
  // the stack to return to state code. This'll change soon (we won't
  // need to push anything to reenter, since the function stack will
  // already be set up for us.)
  MonsterObject * theObj;

  Function *idle;
  MonsterObject *idleObj; // Object owning the idle function
  NodeList * list; // List owning this thread
  int retPos; // Return position in byte code.

  // Stored copy of the stack. Used when the thread is not running.
  int[] sstack;


  public:
  /*******************************************************
   *                                                     *
   *     Public functions                                *
   *                                                     *
   *******************************************************/

  // Get a new thread. It starts in the 'unused' list.
  static Thread* getNew(MonsterObject *obj = null)
  {
    auto cn = scheduler.unused.getNew();
    cn.list = &scheduler.unused;

    with(*cn)
      {
        theObj = obj;

        // Initialize other variables
        idle = null;
        idleObj = null;
        isActive = false;
        stateChange = false;
        retPos = -1;
        sstack = null;
      }

    /*
    if(obj !is null)
      writefln("Got a new state thread");
    else
      writefln("Got a new non-state thread");
    */

    return cn;
  }

  // Unschedule this node from the runlist or waitlist it belongs to,
  // but don't kill it. Any idle function connected to this node is
  // aborted.
  void cancel()
  {
    if(idle !is null)
      {
        fstack.pushIdle(idle, idleObj);
        idle.idleFunc.abort(this);
        fstack.pop();
        idle = null;
      }
    retPos = -1;
    moveTo(&scheduler.unused);

    assert(!isScheduled);
  }

  // Remove the thread comletely
  void kill()
  {
    /*
    if(theObj is null)
      writefln("Killing non-state thread");
    else
      writefln("Killing state thread");
    */

    cancel();
    list.remove(this);
    list = null;

    if(sstack.length)
      Buffers.free(sstack);
    sstack = null;

    /*
    writefln("Thread lists:");
    writefln("  run:     ", scheduler.run.length);
    writefln("  runNext: ", scheduler.runNext.length);
    writefln("  wait:    ", scheduler.wait.length);
    writefln("  unused:  ", scheduler.unused.length);
    */
  }

  bool isDead() { return list is null; }

  // Schedule this thread to run next frame
  void schedule(int offs)
  {
    assert(!isScheduled,
           "cannot schedule an already scheduled thread");

    retPos = offs;
    assert(offs >= 0);
    moveTo(scheduler.runNext);
  }

  // Are we currently scheduled?
  bool isScheduled()
  {
    // The node is per definition scheduled if it is in one of these
    // lists
    return
      list is &scheduler.wait ||
      list is scheduler.run ||
      list is scheduler.runNext;
  }

  bool isUnused()
  {
    return list is &scheduler.unused;
  }

  bool isIdle() { return idle !is null; }

  // Get the next node in the freelist
  Thread* getNext()
  {
    // Simple hack. The Thread (pointed at by the Thread*) is the
    // first part of, and therefore in the same location as, the
    // iterator struct for the FreeList. This is per design, so it's
    // ok to cast the pointer.
    return cast(Thread*)
      ( cast(NodeList.TList.Iterator)this ).getNext();
  }

  // Reenter this thread to the point where it was previously stopped.
  void reenter()
  {
    assert(theObj !is null,
           "cannot reenter a non-state thread yet");

    // Most if not all of these checks will have to be removed in the
    // future
    assert(theObj.state !is null, "attempted to call the empty state");
    assert(!isActive,
           "reenter cannot be called when object is already active");
    assert(fstack.isEmpty,
	   "can only reenter at the bottom of the function stack");
    assert(isScheduled);

    if(isIdle)
      {
        assert(idle !is null);
        assert(idleObj !is null || idle.isStatic);

        // Tell the idle function that we we are reentering
        fstack.pushIdle(idle, idleObj);
        idle.idleFunc.reentry(this);
        fstack.pop();

        // We're no longer idle
        idle = null;
      }

    // Set the active flat to indicate that we are now actively
    // running. (Might not be needed in the future)
    isActive = true;

    // Set the thread
    assert(cthread is null);
    cthread = this;

    // Remove the current thread from the run list
    moveTo(&scheduler.unused);

    // Restore the stack
    restoreStack();

    // Set up the code stack for state code.
    fstack.push(theObj.state, theObj);

    // Set the position
    assert(retPos >= 0);
    fstack.cur.code.jump(retPos);

    // Run the code
    execute();

    // Reset the thread
    cthread = null;

    fstack.pop();

    // We are no longer active
    isActive = false;

    assert(stack.getPos == 0,
           format("Stack not returned to zero after state code, __STACK__=",
                  stack.getPos));

    if(!isUnused)
      // Store the stack
      acquireStack();
    else
      // If the thread is not used for anything, might as well kill it
      kill();
  }

  // Make a copy of the stack and store it for later. Reset the global
  // stack.
  void acquireStack()
  {
    assert(!isUnused(),
           "unused threads should never need to aquire the stack");
    assert(sstack.length == 0,
           "Thread already has a stack");
    assert(fstack.isEmpty);

    // This can be optimized later
    int len = stack.getPos();
    if(len)
      {
        writefln("acquiring %s ints", len);

        // Get a new buffer, and copy the stack
        sstack = Buffers.getInt(len);
        sstack[] = stack.popInts(len);
      }

    stack.reset();
  }

  private:
  /*******************************************************
   *                                                     *
   *     Private helper functions                        *
   *                                                     *
   *******************************************************/

  void restoreStack()
  {
    assert(stack.getPos() == 0,
           "cannot restore into a non-empty stack");

    if(sstack.length)
      {
        // Push the values back, and free the buffer
        stack.pushInts(sstack);
        Buffers.free(sstack);
        assert(stack.getPos == sstack.length);
        sstack = null;
      }
  }

  // Move this node to another list.
  void moveTo(NodeList *to)
  {
    assert(list !is null);
    list.moveTo(*to, this);
    list = to;
  }

  void fail(char[] msg)
  {
    Floc fl;
    if(fstack.cur !is null)
      fl = fstack.cur.getFloc();

    .fail(msg, fl);
  }

  // Parse the BC.CallIdle instruction parameters and schedule the
  // given idle function.
  void callIdle(MonsterObject *iObj)
  {
    assert(isActive && fstack.isStateCode,
           "Byte code attempted to call an idle function outside of state code.");    assert(!isScheduled, "Thread is already scheduled");

    CodeStream *code = &fstack.cur.code;

    // Store the object
    idleObj = iObj;
    assert(idleObj !is null);

    // Get the class from the index
    auto cls = iObj.cls.upcast(code.getInt());

    // Get the function
    idle = cls.findFunction(code.getInt());
    assert(idle !is null && idle.isIdle);
    assert(cls is idle.owner);
    assert(idleObj.cls.childOf(cls));

    // The IdleFunction object bound to this function is stored in
    // idle.idleFunc
    if(idle.idleFunc is null)
      fail("Called unimplemented idle function '" ~ idle.name.str ~ "'");

    // Set the return position
    retPos = fstack.cur.code.getPos();

    // Set up extraData
    extraData = *idleObj.getExtra(idle.owner);

    // Notify the idle function
    fstack.pushIdle(idle, idleObj);
    if(idle.idleFunc.initiate(this))
      moveTo(&scheduler.wait);
    fstack.pop();
  }

  bool shouldExit()
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
    static const long limit = 10000000;

    assert(fstack.cur !is null,
	   "Thread.execute called but there is no code on the function stack.");

    assert(cthread == this,
           "can only run the current thread");

    // Get some values from the function stack
    CodeStream *code = &fstack.cur.code;
    MonsterObject *obj = fstack.cur.obj;
    MonsterClass cls = fstack.cur.getCls();

    // Only an object belonging to this thread can be passed to
    // execute() on the function stack.
    assert(obj is null || cls.parentOf(obj));
    assert(obj !is null || fstack.cur.isStatic);

    // Pops a pointer off the stack. Null pointers will throw an
    // exception.
    int *popPtr()
      {
        PT type;
        int index;
        decodePtr(stack.popInt(), type, index);

        // Null pointer?
        if(type == PT.Null)
          fail("Cannot access value, null pointer");
  
        // Stack variable?
        if(type == PT.Stack)
          return stack.getFrameInt(index);

        // Variable in this object
        if(type == PT.DataOffs)
          return obj.getDataInt(cls.treeIndex, index);

        // This object, but another (parent) class
        if(type == PT.DataOffsCls)
          // We have to pop the class index of the stack as well
          return obj.getDataInt(stack.popInt, index);

        // Far pointer, with offset. Both the class index and the object
        // reference is on the stack.
        if(type == PT.FarDataOffs)
          {
            int clsIndex = stack.popInt();

            // Get the object reference from the stack
            MonsterObject *tmp = stack.popObject();

            // Return the correct pointer
            return tmp.getDataInt(clsIndex, index);
          }

        // Array pointer
        if(type == PT.ArrayIndex)
          {
            assert(index==0);
            // Array indices are on the stack
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

    // Various temporary stuff
    int *ptr;
    long *lptr;
    float *fptr;
    double *dptr;
    int[] iarr;
    ArrayRef *arf;
    int val, val2;
    long lval;

    // Disable this for now.
    // or at least a compile time option.
    //for(long i=0;i<limit;i++)
    for(;;)
      {
	ubyte opCode = code.get();

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
              // Get the correct function from the virtual table
              val = code.getInt(); // Class index
              auto fn = obj.cls.findVirtualFunc(val, code.getInt());

              // Finally, call
              fn.call(obj);

              if(shouldExit()) return;
              break;
            }

          case BC.CallFar:
            {
              auto mo = stack.popObject();

              // Get the correct function from the virtual table
              val = code.getInt(); // Class index
              auto fn = mo.cls.findVirtualFunc(val, code.getInt());

              // Call the function
              fn.call(mo);

              // Exit state code if the state was changed
              if(shouldExit()) return;
              break;
            }

          case BC.CallIdle:
            callIdle(obj);
            return;

          case BC.CallIdleFar:
            callIdle(stack.popObject());
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
	    obj.setState(val, val2, code.getInt());
            if(shouldExit()) return;
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
	    stack.pushInt(*obj.getDataInt(cls.treeIndex, code.getInt()));
	    break;

          case BC.PushParentVar:
            // Get the tree index
            val = code.getInt();
            stack.pushInt(*obj.getDataInt(val, code.getInt()));
            break;

	  case BC.PushFarClassVar:
            {
              // Get object to work on
              MonsterObject *mo = stack.popObject();
              // And the tree index
              val = code.getInt();
              stack.pushInt(*mo.getDataInt(val, code.getInt()));
            }
	    break;

	  case BC.PushFarClassMulti:
            {
              int siz = code.getInt(); // Variable size
              // Get object to work on
              MonsterObject *mo = stack.popObject();
              // And the tree index
              val = code.getInt();  // Class tree index
              val2 = code.getInt(); // Data segment offset
              stack.pushInts(mo.getDataArray(val,val2,siz));
            }
	    break;

	  case BC.PushThis:
	    // Push the index of this object.
	    stack.pushObject(obj);
	    break;

	  case BC.PushSingleton:
	    stack.pushObject(global.getClass(cast(CIndex)code.getInt).getSing);
	    break;

	  case BC.Pop: stack.popInt(); break;

	  case BC.PopN: stack.pop(code.get()); break;

	  case BC.Dup: stack.pushInt(*stack.getInt(0)); break;

	  case BC.StoreRet:
	    // Get the pointer off the stack, and convert it to a real
	    // pointer.
	    ptr = popPtr();
	    // Read the value and store it, but leave it in the stack
	    *ptr = *stack.getInt(0);
	    break;

	  case BC.StoreRet8:
            ptr = popPtr();
            *(cast(long*)ptr) = *stack.getLong(1);
            break;

          case BC.StoreRetMult:
            val = code.getInt(); // Size
            ptr = popPtr();
            ptr[0..val] = stack.getInts(val-1, val);
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
	    ptr = popPtr();
	    stack.pushInt(++(*ptr));
	    break;

	  case BC.PreDec:
	    ptr = popPtr();
	    stack.pushInt(--(*ptr));
	    break;

	  case BC.PostInc:
	    ptr = popPtr();
	    stack.pushInt((*ptr)++);
	    break;

	  case BC.PostDec:
	    ptr = popPtr();
	    stack.pushInt((*ptr)--);
	    break;

	  case BC.PreInc8:
	    lptr = cast(long*)popPtr();
	    stack.pushLong(++(*lptr));
	    break;

	  case BC.PreDec8:
	    lptr = cast(long*)popPtr();
	    stack.pushLong(--(*lptr));
	    break;

	  case BC.PostInc8:
	    lptr = cast(long*)popPtr();
	    stack.pushLong((*lptr)++);
	    break;

	  case BC.PostDec8:
	    lptr = cast(long*)popPtr();
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

          case BC.CastT2S:
            {
              // Get the type to cast from
              val = code.getInt();
              Type t = Type.typeList[val];
              // Get the data
              iarr = stack.popInts(t.getSize());
              // Let the type convert to string
              char[] str = t.valToString(iarr);
              // And push it back
              stack.pushArray(str);
            }
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

// The scheduler singleton
Scheduler scheduler;

struct Scheduler
{
  // Run lists - threads that run this or the next round.
  NodeList run1, run2;

  // Waiting list - idle threads that are actively checked each frame.
  NodeList wait;

  // List of unused nodes. Any thread in this list (that is not
  // actively running) can and will be deleted eventually.
  NodeList unused;

  // The run lists for this and the next round. We use pointers to the
  // actual lists, since we want to swap them easily.
  NodeList* runNext, run;

  void init()
  {
    // Assign the run list pointers
    run = &run1;
    runNext = &run2;
  }

  // Statistics:

  // Number of elements in the waiting list
  int numWait() { return wait.length; }

  // Number of elements scheduled to run the next frame
  int numRun() { return runNext.length; }

  // Number of remaining elements this frame
  int numLeft() { return run.length; }

  // Total number of objects scheduled or waiting
  int numTotal()
  { return numRun() + numWait() + numLeft(); }

  // Do a complete frame. TODO: Make a distinction between a round and
  // a frame later. We could for example do several rounds per frame,
  // measured by some criterion of how much time we want to spend on
  // script code or whether there are any pending items in the run
  // list. We could do several runs of the run-list (to handle state
  // changes etc) but only one run on the condition list (actually
  // that is a good idea.) We also do not have to execute everything in
  // the run list if it is long (otoh, allowing a build-up is not
  // good.) But all this falls in the "optimization" category.
  void doFrame()
  {
    checkConditions();
    dispatch();
  }

  void checkConditions()
  {
    // Go through the condition list for this round.
    Thread* cn = wait.getHead();
    Thread* next;
    while(cn != null)
      {
	// Get the next node here, since the current node might move
	// somewhere else during this iteration, and then getNext will
	// point to another list.
	next = cn.getNext();

        assert(cn.isScheduled);

	// This is an idle function and it is finished. Note that
	// hasFinished() is NOT allowed to change the wait list in any
	// way, ie to change object states or interact with the
	// scheduler. In fact, hasFinished() should do as little as
	// possible.
	if(cn.isIdle)
	  {
            fstack.pushIdle(cn.idle, cn.idleObj);
            if(cn.idle.idleFunc.hasFinished(cn))
              // Schedule the code to start running again this round. We
              // move it from the wait list to the run list.
              cn.moveTo(runNext);

            fstack.pop();
	  }
	// Set the next item
	cn = next;
      }
  }

  void dispatch()
  {
    // Swap the runlist for the next frame with the current one. All
    // code that is scheduled after this point is executed the next
    // frame.
    auto tmp = runNext;
    runNext = run;
    run = tmp;

    // Now execute the run list for this frame. Note that items might
    // be removed from the run list as we go (eg. if a scheduled
    // object has it's state changed) but this is handled. New nodes
    // might also be scheduled, but these are added to the runNext
    // list.

    // First element
    Thread* cn = run.getHead();
    while(cn != null)
      {
        // Execute
        cn.reenter();

        // The function stack should now be at zero
        assert(fstack.isEmpty());

	// Get the next item.
	cn = run.getHead();
      }

    // Check that we cleared the run list
    assert(run.length == 0);
  }
}
