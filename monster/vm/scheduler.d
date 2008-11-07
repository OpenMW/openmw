/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (scheduler.d) is part of the Monster script language
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

module monster.vm.scheduler;

import monster.compiler.functions;

import monster.vm.mobject;
import monster.vm.idlefunction;
import monster.vm.error;
import monster.vm.fstack;

import monster.util.freelist;
import monster.minibos.string;

// Enable minor safety checks - can be removed from release code.
debug=safecheck;

// Are we currently looping through the wait list?
debug(safecheck) bool waitLoop;

// The various types of code a scheduled node will call
enum CallType
  {
    None,	// Not used and should never be set
    Idle,	// The return of an idle function (starts state code)
    State	// The beginning of a state (also starts state code)
  }

// The scheduler singleton
Scheduler scheduler;

// Represents a code point for the scheduler to jump back to. Points
// to an object (which owns a coe thread object) and a position. TODO:
// Function calls must refer to some index, if the state changes we
// must call the correct function.
struct ScheduleStruct
{
  CallType type;
  Function *idle;
  MonsterObject *obj;
  ListManager *list;
  int retPos; // Return position for idle functions.

  // Unschedule this node from the runlist or waitlist it belongs
  // to. Any idle function connected to this node is aborted.
  void cancel()
  {
    debug(safecheck) auto node = obj.thread.scheduleNode;
    if(idle !is null)
      {
        fstack.pushIdleAbort(idle, obj);
        idle.idleFunc.abort(obj);
        fstack.pop();
      }
    // Make sure the scheduleNode is the same before and after calling
    // abort().
    debug(safecheck)
      assert(node == obj.thread.scheduleNode,
	     "abort() can not reschedule object or change state");
    remove();
  }

  // Remove this node from the list it belongs to.
  void remove()
  {
    debug(safecheck) assert(!waitLoop, "remove() called from hasFinished()");
    type = CallType.None;
    list.remove(obj);
  }
}

alias FreeList!(ScheduleStruct) ScheduleFreeList;
alias ScheduleStruct* CallNode;

// Get the next node in a freelist
static CallNode getNext(CallNode cn)
{
  // Simple hack. The ScheduleStruct (pointed at by the CallNode) is
  // the first part of, and therefore in the same location as, the
  // iterator struct for the FreeList. It's therefore ok to cast the
  // pointer, as long as we never change the iterator struct layout.
  return cast(CallNode)
    ( cast(ScheduleFreeList.TList.Iterator)cn ).getNext();
}

// A wrapper around a freelist. This struct takes care of some
// additional pointers in CodeThread and in ScheduleStruct.
struct ListManager
{
  ScheduleFreeList list;

  // Create a new node in this list. This means scheduling the given
  // object in one of the lists.
  CallNode newNode(MonsterObject *obj, CallType type,
		   Function *idle, int retPos)
  {
    assert(obj.thread.scheduleNode == null,
	   "CodeThread cannot have two schedule nodes.");

    CallNode cn = list.getNew();
    cn.obj = obj;
    cn.type = type;
    cn.idle = idle;
    cn.list = this;
    cn.retPos = retPos;
    obj.thread.scheduleNode = cn;
    return cn;
  }

  // Remove a node from this run list (put it back into the freelist.)
  void remove(MonsterObject *obj)
  {
    CallNode node = obj.thread.scheduleNode;
    node.list = null;
    obj.thread.scheduleNode = null;
    list.remove(node);
  }

  CallNode moveTo(ListManager *to, CallNode node)
  {
    node.list = to;
    return list.moveTo(to.list, node);
  }
}

struct Scheduler
{
  // The acutal lists. We use pointers to access the run lists, since
  // we want to swap them easily.
  ListManager run1, run2, wait;

  // The run lists for this and the next round.
  ListManager* runNext, run;

  void init()
  {
    // Assign the run list pointers
    run = &run1;
    runNext = &run2;
  }

  // Statistics:

  // Number of elements in the waiting list
  int numWait() { return wait.list.length; }

  // Number of elements scheduled to run the next frame
  int numRun() { return runNext.list.length; }

  // Total number of objects scheduled or waiting
  int numTotal()
  {
    assert(run.list.length == 0); // The 'run' list is always empty
                                  // between frames.
    return numRun() + numWait();
  }

  // "Call" an idle function. We must notify the idle function that it
  // has been called. We also have the responsibility of rescheduling
  // the given code thread at the right moment.
  void callIdle(MonsterObject *obj, Function *idle,
		int pos)
  {
    //writefln("Idle function '%s' called", lf.name);
    debug(safecheck) assert(!waitLoop, "callIdle() called from hasFinished()");
    assert(obj.thread.scheduleNode == null);

    // Make sure the object and the function are set up correctly
    assert(obj.cls is idle.owner);
    assert(idle.isIdle);
    assert(idle.idleFunc !is null);

    // Notify the idle function
    fstack.pushIdleInit(idle, obj);
    if(idle.idleFunc.initiate(obj))
      {
        // The idle function wants to be scheduled

        // Make sure initiate() didn't change anything it shouldn't
        // have.
        assert(obj.thread.scheduleNode == null,
               "initiate() cannot reschedule object or change state");

        // Schedule it to be checked next round.
        wait.newNode(obj, CallType.Idle, idle, pos);
      }
    
    fstack.pop();
  }

  // Schedule the given state code to run the next frame
  void scheduleState(MonsterObject *obj, int offs)
  {
    runNext.newNode(obj, CallType.State, null, offs);
  }

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
    //writefln("Beginning of frame");

    // Turn on some safety features
    debug(safecheck) waitLoop = true;

    // Go through the condition list for this round.
    CallNode cn = wait.list.getHead();
    CallNode next;
    while(cn != null)
      {
	// Get the next node here, since the current node might move
	// somewhere else during this iteration, and then getNext will
	// point to another list.
	next = getNext(cn);

	assert(cn.obj.thread.scheduleNode == cn);

	// This is an idle function and it is finished. Note that
	// hasFinished() is NOT allowed to change the wait list in any
	// way, ie to change object states or interact with the
	// scheduler. In fact, hasFinished() should do as little as
	// possible.
	if(cn.type == CallType.Idle)
	  {
            fstack.pushIdleCheck(cn.idle, cn.obj);
            if(cn.idle.idleFunc.hasFinished(cn.obj))
              // Schedule the code to start running again this round. We
              // move it from the wait list to the run list.
              wait.moveTo(runNext,cn);

            fstack.pop();
	  }
	// Set the next item
	cn = next;
      }

    debug(safecheck) waitLoop = false;


    /*
    if(wait.list.length)
      writefln("  There are idle functions lurking in the shadows");
    */

    //writefln("Condition phase complete, beginning execution phase.");

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
    cn = run.list.getHead();
    while(cn != null)
      {
	// Remove the current item from the list before starting.
        MonsterObject *obj = cn.obj;
	auto code = obj.thread;
	assert(code.scheduleNode == cn);
	run.remove(obj);

	// Now execute the item
	if(cn.type == CallType.Idle)
	  {
	    // Tell the idle function that we we are reentering
            fstack.pushIdleReentry(cn.idle, obj);
	    cn.idle.idleFunc.reentry(obj);
            fstack.pop();

	    assert(code.scheduleNode == null,
		   "reentry() cannot reschedule object or change state");

	    // Return to the code point
	    code.callState(cn.retPos);
	  }
	else if(cn.type == CallType.State)
	  // Code is scheduled after a state change. We must jump to
	  // the right offset.
	  code.callState(cn.retPos);

	else assert(0, "Unhandled return type");

        // The function stack should now be at zero
        assert(fstack.isEmpty());

	// Get the next item.
	cn = run.list.getHead();
      }

    // Check that we cleared the run list
    assert(run.list.length == 0);

    //writefln("End of frame\n");
  }
}
