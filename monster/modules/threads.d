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
idle resume();

// Wait for a thread to finish. Will not return until the thread is
// dead.
idle wait();

// Call a function as a thread
thread call(char[] name)
{
  var t = create(name);
  t.resume();
  return t;
}

// Start a function as a thread in the background
thread start(char[] name)
{
  var t = create(name);
  t.restart();
  return t;
}
"; //"

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

  // Find the function
  auto fn = mo.cls.findFunction(name);

  if(fn.paramSize > 0)
    fail("create(): function " ~ name ~ " cannot have parameters");

  // Create a new thread
  Thread *trd = Thread.getNew();

  // Schedule the thread run the next frame
  trd.pushFunc(fn, mo);
  assert(trd.isPaused);

  // This will mess with the stack frame though, so set it up
  // correctly.
  trd.fstack.cur.frame = stack.getStartInt(0);
  cthread.fstack.restoreFrame();

  stack.pushObject(createObj(trd));
}

// Resume is used to restore a thread that was previously paused. It
// will enter the thread immediately, like call. If you wish to run it
// later, use restart instead.
class Resume : IdleFunction
{
 override:
  IS initiate(Thread *t)
    {
      if(params.obj is trdSing)
        fail("Cannot use resume() on our own thread.");

      // Get the thread we're resuming
      auto trd = getOwner();

      if(trd is t)
        fail("Cannot use resume() on our own thread.");

      if(trd.isDead)
        fail("Cannot resume a dead thread.");

      if(!trd.isPaused)
        fail("Can only use resume() on paused threads");

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
{
  auto trd = getOwner();

  if(trd.isDead)
    fail("Cannot restart a dead thread");

  if(!trd.isPaused)
    fail("Can only use restart() on paused threads");

  // Move to the runlist
  trd.moveTo(scheduler.runNext);
}

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
  _threadClass.bind("resume", new Resume);
  _threadClass.bind("pause", new Pause);
  _threadClass.bind("wait", new Wait);

  _threadClass.bind("create", &create);
  _threadClass.bind("restart", &restart);

  _threadClass.bind("isDead", &isDead);
  _threadClass.bind("isIdle", &isIdle);
  _threadClass.bind("isPaused", &isPaused);
  _threadClass.bind("isScheduled", &isScheduled);
}
