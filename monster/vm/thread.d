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

  // The contents of the idle object's extra data for the idle's owner
  // class.
  SharedType extraData;

  // Set to true when a state change is in progress. Only used when
  // state is changed from within a function in active code.
  bool shouldExit;

  // Function stack for this thread
  FunctionStack fstack;

  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/

  private:
  NodeList *list; // List owning this thread

  // Stored copy of the stack. Used when the thread is not running.
  int[] sstack;

  public:
  /*******************************************************
   *                                                     *
   *     Public functions                                *
   *                                                     *
   *******************************************************/

  // Get a new thread. It starts in the 'transient' list.
  static Thread* getNew()
  {
    auto cn = scheduler.transient.getNew();
    cn.list = &scheduler.transient;

    with(*cn)
      {
        // Initialize other variables
        shouldExit = false;
        sstack = null;
      }

    return cn;
  }

  // Schedule the function to run the next frame. Can only be used on
  // paused threads.
  void restart()
  {
    if(isDead)
      fail("Cannot restart a dead thread");

    if(!isPaused)
      fail("Can only use restart() on paused threads");

    // Move to the runlist
    moveTo(scheduler.runNext);
  }

  // Stop the thread and return it to the freelist
  void kill()
  {
    stop();
    assert(fstack.isEmpty);
    list.remove(this);
    list = null;
    assert(isDead);
  }

  // Stop the execution of a thread and cancel any scheduling.
  void stop()
  {
    assert(!isDead);

    // TODO: We also have to handle (forbid) cases where we are
    // the caller of another thread.

    if(isRunning)
      {
        // We are running.
        assert(sstack.length == 0);

        // Forbid stopping the thread if there are native functions on
        // the function stack.
        if(fstack.hasNatives)
          fail("Cannot stop thread, there are native functions on the stack.");

        // Kill the stack tell execute() to stop running
        stack.reset();
        shouldExit = true;
      }
    else
      {
        // We are not running

        // Free the stack buffers
        if(sstack.length)
          Buffers.free(sstack);
        sstack = null;

        // Abort any idle function
        if(fstack.isIdle)
          {
            // Abort the idle function and pop it
            getIdle().abort(this);
            fstack.pop();
          }
        assert(!fstack.hasNatives);
      }

    // Kill the function stack
    fstack.killAll();

    // Move to the transient list (signalling that the thread is
    // unused.)
    moveTo(&scheduler.transient);
    assert(!isScheduled);
  }

  // Schedule this thread to run state code the next frame
  void scheduleState(MonsterObject *obj, int offs)
  {
    assert(!isDead);
    assert(!isScheduled,
           "cannot schedule an already scheduled thread");
    assert(!fstack.isIdle);
    assert(fstack.isEmpty);
    assert(offs >= 0);
    assert(obj !is null);

    assert(isRunning == shouldExit);

    // Move to the runlist
    moveTo(scheduler.runNext);

    // Set up the function stack
    fstack.push(obj.state, obj);
    fstack.cur.code.jump(offs);
  }

  // Push a function and pause the thread
  void pushFunc(Function *fn, MonsterObject *obj)
  {
    assert(!isDead);
    assert(!isScheduled,
           "cannot schedule an already scheduled thread");
    assert(fstack.isEmpty);
    assert(!fstack.isIdle);
    assert(fn !is null);
    assert(obj !is null);
    assert(!fn.isIdle);

    // Set up the function stack
    assert(fn.owner.parentOf(obj));
    fstack.push(fn, obj);

    moveTo(&scheduler.paused);
  }

  // Are we currently scheduled?
  bool isScheduled()
  {
    // The node is per definition scheduled if it is in one of these
    // lists
    return
      !isDead && (list is &scheduler.wait ||
                  list is scheduler.run ||
                  list is scheduler.runNext);
  }

  bool isTransient() { return list is &scheduler.transient; }
  bool isRunning() { return cthread is this; }
  bool isDead() { return list is null; }
  bool isAlive() { return !isDead; }
  bool isPaused() { return list is &scheduler.paused; }

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
    assert(!isDead);
    assert(cthread is null,
           "cannot reenter when another thread is running");
    assert(!isRunning,
           "reenter cannot be called when thread is already running");
    assert(isScheduled || isPaused);
    assert(!fstack.isEmpty);

    // Put the thread in the foreground
    foreground();
    assert(isRunning);

    // Notify the idle function, if any
    if(fstack.isIdle)
      {
        // Tell the idle function that we we are reentering
        assert(fstack.isIdle);
        getIdle().reentry(this);

        // Remove the idle function
        fstack.pop();
      }

    assert(fstack.cur.isNormal,
           "can only reenter script code");

    // Remove the current thread from the run list
    moveTo(&scheduler.transient);

    // Run the code
    execute();

    // Exit immediately if the thread committed suicide.
    if(isDead) return;

    shouldExit = false;

    // Background the thread
    background();
    assert(cthread is null);
  }

  // Put this thread in the background. Acquires the stack and
  // function stack.
  void background()
  {
    assert(!isDead);
    assert(sstack.length == 0,
           "Thread already has a stack");
    assert(isRunning,
           "cannot put a non-running thread in the background");
    assert(!fstack.hasNatives);

    // We're no longer the current thread
    cthread = null;

    if(isTransient)
      {
        // The thread is not scheduled and will not be used
        // anymore. Might as well kill it.
        assert(!isRunning);
        kill();
      }
    else
      {
        // The thread will possibly be restored at some point. Store
        // the stack and fstack for later.

        // Stack.
        int len = stack.getPos();
        if(len)
          {
            // Get a new buffer, and copy the stack
            sstack = Buffers.getInt(len);
            sstack[] = stack.popInts(len);
          }
      }

    // Clear out our stack values
    stack.reset();

    assert(!isRunning);
  }

  // Put the thread in the foreground. Restore any stored stack
  // values.
  void foreground()
  {
    assert(!isDead);
    assert(cthread is null,
           "cannot restore thread, another thread is running");
    assert(!isRunning,
           "cannot restore thread, it is already running");

    assert((isTransient && fstack.isEmpty) ||
           stack.getPos() == 0,
           "only empty transient threads kan restore into a non-empty stack");

    if(sstack.length)
      {
        assert(stack.getPos() == 0,
               "cannot restore into a non-empty stack");
        assert(!isTransient,
               "cannot restore a transent thread with stack");

        // Push the values back, and free the buffer
        stack.pushInts(sstack);
        Buffers.free(sstack);
        assert(stack.getPos == sstack.length);
        sstack = null;
      }

    // Restore the stack frame pointer
    fstack.restoreFrame();

    // Set ourselves as the running thread
    cthread = this;
  }

  // Move this node to another list.
  void moveTo(NodeList *to)
  {
    assert(list !is null);
    list.moveTo(*to, this);
    list = to;
  }

  private:
  /*******************************************************
   *                                                     *
   *     Private helper functions                        *
   *                                                     *
   *******************************************************/

  IdleFunction getIdle()
  {
    assert(fstack.isIdle);
    assert(fstack.cur.func !is null);
    assert(fstack.cur.func.idleFunc !is null);
    return fstack.cur.func.idleFunc;
  }

  void fail(char[] msg)
  {
    Floc fl;
    if(fstack.cur !is null)
      fl = fstack.cur.getFloc();

    msg ~= '\n' ~ fstack.toString();

    .fail(msg, fl);
  }

  // Parse the BC.CallIdle instruction parameters and schedule the
  // given idle function. Return true if we should exit execute()
  bool callIdle(MonsterObject *iObj)
  {
    assert(isRunning);
    assert(!isScheduled, "Thread is already scheduled");
    assert(iObj !is null);

    if(fstack.hasNatives)
      fail("Cannot run idle function: there are native functions on the stack");

    CodeStream *code = &fstack.cur.code;

    // Get the class from the index
    auto cls = iObj.cls.upcast(code.getInt());

    // Get the function
    Function *idle = cls.findFunction(code.getInt());
    assert(idle !is null && idle.isIdle);
    assert(cls is idle.owner);
    assert(iObj.cls.childOf(cls));

    // The IdleFunction object bound to this function is stored in
    // idle.idleFunc
    if(idle.idleFunc is null)
      fail("Called unimplemented idle function '" ~ idle.name.str ~ "'");

    // Set up extraData
    extraData = *iObj.getExtra(idle.owner);

    // Push the idle function on the stack, with iObj as the 'this'
    // object.
    fstack.pushIdle(idle, iObj);

    // Notify the idle function that it was called
    auto res = idle.idleFunc.initiate(this);
    //writefln("Called %s, result was %s", idle.name, res);

    if(res == IS.Poll)
      {
        moveTo(&scheduler.wait);
        return true;
      }

    if(res == IS.Return)
      {
        // If we're returning, call reenter immediately
        idle.idleFunc.reentry(this);

        // The function is done, pop it back of the stack
        fstack.pop();

        // 'false' means continue running
        return false;
      }

    assert(res == IS.Manual || res == IS.Kill);

    // The only difference between Manual and Kill is what list the
    // thread ends in. If the thread is in the transient list, it will
    // be killed automatically when it's no longer running.
    assert( (res == IS.Kill) == isTransient,
            res == IS.Manual ? "Manually scheduled threads must be moved to another list." : "Killed threads cannot be moved to another list.");

    // 'true' means exit execute()
    return true;
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

    assert(!isDead);
    assert(fstack.cur !is null,
	   "Thread.execute called but there is no code on the function stack.");
    assert(fstack.cur.isNormal,
           "execute() can only run script code");
    assert(isRunning,
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
            // Step down once on the function stack
            fstack.pop();

            if(!fstack.isNormal())
              // The current function isn't a script function, so
              // exit.
              return;

            assert(!shouldExit);

            // Set up the variables and continue running.
            assert(fstack.cur !is null);
            cls = fstack.cur.getCls();
            code = &fstack.cur.code;
            obj = fstack.cur.obj;

            break;

            // Start a block so these variables are local
            {
              MonsterObject *mo;
              Function *fn;

            case BC.Call:
              mo = obj;
              goto CallCommon;

            case BC.CallFar:
              mo = stack.popObject();

            CallCommon:

              // Get the correct function from the virtual table
              val = code.getInt(); // Class index
              fn = mo.cls.findVirtualFunc(val, code.getInt());

              if(fn.isNormal)
                {
                  // Normal (script) function. We don't need to exit
                  // execute(), just change the function stack and the
                  // cls and code pointers. Then keep running.
                  fstack.push(fn, mo);
                  cls = fstack.cur.getCls();
                  code = &fstack.cur.code;
                  obj = mo;
                  assert(obj is fstack.cur.obj);
                }
              else
                {
                  // Native function. Let Function handle it.
                  assert(fn.isNative);
                  fn.call(mo);
                  if(shouldExit) return;
                }
              break;
            }

          case BC.CallIdle:
            if(callIdle(obj))
              return;
            break;

          case BC.CallIdleFar:
            if(callIdle(stack.popObject()))
              return;
            break;

	  case BC.Return:
	    // Remove the given number of bytes from the stack, and
	    // exit the function.
	    stack.pop(code.getInt());
            goto case BC.Exit;

	  case BC.ReturnVal:
	    stack.pop(code.getInt(), 1);
            goto case BC.Exit;

	  case BC.ReturnValN:
	    val = code.getInt(); // Get the value first, since order
				 // of evaluation is important.
	    stack.pop(val, code.getInt());
            goto case BC.Exit;

	  case BC.State:
            val = code.getInt(); // State index
            val2 = code.getInt(); // Label index
	    // Get the class index and let setState handle everything
	    obj.setState(val, val2, code.getInt());
            if(shouldExit) return;
	    break;

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

  // List of transient nodes. Any thread in this list (that is not
  // actively running) can and will be deleted when it goes into the
  // background.
  NodeList transient;

  // List of threads that are not running or scheduled, but should not
  // be deleted.
  NodeList paused;

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
    assert(cthread is null,
           "cannot run doFrame while another thread is running");
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
	if(cn.fstack.isIdle)
	  {
            if(cn.getIdle().hasFinished(cn))
              // Schedule the code to start running again this round. We
              // move it from the wait list to the run list.
              cn.moveTo(runNext);
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

	// Get the next item.
	cn = run.getHead();
      }

    // Check that we cleared the run list
    assert(run.length == 0);
  }
}
