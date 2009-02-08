/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (threads.d) is part of the Monster script language
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

// This module provides an interface to the virtual threading API in
// Monster.

module monster.modules.threads;

import monster.monster;
import std.stdio;

const char[] moduleDef =
"singleton thread;

// Used to kill or pause our own or other threads.
idle kill();
idle pause();

// Get status information about a thread
native bool isScheduled();
native bool isPaused();
native bool isIdle();
native bool isDead();
bool isAlive() { return !isDead(); }

// Create a new (paused) thread for a given function
native thread create(char[] name);

// Schedule a (paused) thread to run the next frame
native restart();

// Call a (paused) thread directly - returns when the thread exits or
// calls an idle function.
idle call();

// Wait for a thread to finish. Will not return until the thread is
// dead.
idle wait();

// Start a function as a thread in the background
thread start(char[] name)
{
  var t = create(name);
  t.restart();
  return t;
}
"; //"

/*
  The char[] name stuff above will of course be replaced with real
  function pointers once those are done. We will also add:

  function() wrap(function f())
  {
    var t = create(f);
    return {{ t.call(); }
  }

*/

MonsterObject *trdSing;

class Kill : IdleFunction
{
  IS initiate(Thread *t)
    {
      auto mo = params.obj;

      if(mo !is trdSing)
        {
          // Check if this is another thread
          auto trd = getOwner(mo);
          if(trd !is t)
            {
              // It is. Kill it explicitly and return.
              trd.kill();
              return IS.Return;
            }
        }

      // If it's our own thread, tell the scheduler to kill it from
      // the inside.
      return IS.Kill;
    }
}

class Pause : IdleFunction
{
  IS initiate(Thread *t)
    {
      auto mo = params.obj;

      // Can only run on the singleton object
      if(mo !is trdSing)
        fail("Can only pause our own thread");

      // Move the thread to the 'paused' list
      t.moveTo(&scheduler.paused);

      return IS.Manual;
    }
}

Thread *getOwner() { return getOwner(params.obj); }
Thread *getOwner(MonsterObject *mo)
{
  assert(mo !is null);
  if(mo is trdSing)
    fail("Cannot run this function on the global singleton thread.");
  auto trd = cast(Thread*) mo.getExtra(_threadClass).vptr;
  assert(trd !is null);
  assert(!trd.isRunning || !trd.isPaused,
         "thread cannot be running and paused at the same time");
  return trd;
}

MonsterObject *createObj(Thread *trd)
{
  assert(trd !is null);
  auto mo = _threadClass.createObject();
  mo.getExtra(_threadClass).vptr = trd;
  return mo;
}

void create()
{
  // Can only run on the singleton object
  if(params.obj !is trdSing)
    fail("Can only use create() on the global thread object.");

  char[] name = stack.popString8();

  assert(cthread !is null);

  // This is a dirty hack that's only needed because we haven't
  // implemented function pointers yet. Gets the caller object.
  assert(cthread.fstack.list.length >= 2);
  auto nd = cthread.fstack.cur;
  nd = cthread.fstack.list.getNext(nd);
  if(nd.getCls() is _threadClass)
    nd = cthread.fstack.list.getNext(nd);
  auto mo = nd.obj;

  auto trd = mo.thread(name);

  stack.pushObject(createObj(trd));
}

// Call is used to restore a thread that was previously paused. It
// will enter the thread immediately, like a normal function call, but
// it will still run in its own thread. If you only wish to schedule
// it for later, use restart instead.
class Call : IdleFunction
{
 override:
  IS initiate(Thread *t)
    {
      if(params.obj is trdSing)
        fail("Cannot use call() on our own thread.");

      // Get the thread we're resuming
      auto trd = getOwner();

      if(trd is t)
        fail("Cannot use call() on our own thread.");

      if(trd.isDead)
        fail("Cannot call a dead thread.");

      if(!trd.isPaused)
        fail("Can only use call() on paused threads");

      // Background the current thread. Move it to the pause list
      // first, so background doesn't inadvertently delete it.
      t.moveTo(&scheduler.paused);
      t.background();
      assert(!t.isDead);

      // Reenter the thread
      trd.reenter();
      assert(cthread is null);

      // Put the old thread in the forground again
      t.foreground();

      // Make the thread transient again
      t.moveTo(&scheduler.transient);

      // Return to sender
      return IS.Return;
    }

  void abort(Thread *t)
    {
      fail("Cannot abort thread while it is calling another thread");
    }
}

class Wait : IdleFunction
{
 override:
  IS initiate(Thread *t)
    {
      if(params.obj is trdSing)
        fail("Cannot use wait on our own thread.");

      // Get the thread we're resuming
      auto trd = getOwner();

      if(trd is t)
        fail("Cannot use wait on our own thread.");

      // Return immediately if the thread is dead
      if(trd.isDead)
        return IS.Return;

      t.idleData.vptr = trd;

      return IS.Poll;
    }

  bool hasFinished(Thread *t)
    {
      return (cast(Thread*)t.idleData.vptr).isDead;
    }
}

void restart()
{ getOwner().restart(); }

void isDead()
{ stack.pushBool(getOwner().isDead); }

void isIdle()
{ stack.pushBool(getOwner().fstack.isIdle); }

void isPaused()
{ stack.pushBool(getOwner().isPaused); }

void isScheduled()
{ stack.pushBool(getOwner().isScheduled); }

MonsterClass _threadClass;

void initThreadModule()
{
  if(_threadClass !is null)
    return;

  _threadClass = new MonsterClass(MC.String, moduleDef, "thread");
  trdSing = _threadClass.getSing();

  _threadClass.bind("kill", new Kill);
  _threadClass.bind("call", new Call);
  _threadClass.bind("pause", new Pause);
  _threadClass.bind("wait", new Wait);

  _threadClass.bind("create", &create);
  _threadClass.bind("restart", &restart);

  _threadClass.bind("isDead", &isDead);
  _threadClass.bind("isIdle", &isIdle);
  _threadClass.bind("isPaused", &isPaused);
  _threadClass.bind("isScheduled", &isScheduled);
}
