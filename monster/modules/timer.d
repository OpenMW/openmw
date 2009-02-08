/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (timer.d) is part of the Monster script language
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

// This module contains (or will contain) various routines for
// timing. It is also home of the ubiquitous "sleep" idle function.

module monster.modules.timer;

import std.stdio;
import std.date;

// For some utterly idiotic reason, DMD's public imports will suddenly
// stop working from time to time.
import monster.vm.mclass;
import monster.vm.mobject;
import monster.vm.stack;
import monster.vm.thread;
import monster.vm.idlefunction;

import monster.monster;

const char[] moduleDef =
"singleton timer;
idle sleep(float secs);
"; //"

// Sleep a given amount of time. This implementation uses the system
// clock and is the default.
class IdleSleep_SystemClock : IdleFunction
{
 override:
  IS initiate(Thread* cn)
    {
      // Get the parameter
      float secs = stack.popFloat;

      // Get current time
      cn.idleData.l = getUTCtime();

      // Calculate when we should return
      cn.idleData.l += secs*TicksPerSecond;

      // Schedule us
      return IS.Poll;
    }

  bool hasFinished(Thread* cn)
    {
      // Is it time?
      return getUTCtime() >= cn.idleData.l;
    }
}

// This implementation uses a user-driven timer instead of the system
// clock. It's more efficient, but requires the user to update the
// given timer manuall each frame. The default sleep (timer.sleep) is
// bound to the default timer, but it's possible to create multiple
// independent timers.
class IdleSleep_Timer : IdleFunction
{
 override:
  IS initiate(Thread* cn)
    {
      // The timer is stored in the object's 'extra' pointer
      auto t = cast(SleepManager)cn.extraData.obj;
      assert(t !is null);

      // Calculate the return time
      cn.idleData.l = t.current + cast(long)(t.tickSize*stack.popFloat);

      // Schedule us
      return IS.Poll;
    }

  bool hasFinished(Thread* cn)
    {
      // Get the timer
      auto t = cast(SleepManager)cn.extraData.obj;
      assert(t !is null);

      // Is it time?
      return t.current >= cn.idleData.l;
    }
}

// A manually updated timer. This can be improved quite a lot: Most
// sleep operations (depending on application of course) will skip
// many frames before they return. For example, for sleep(0.5) at 100
// fps, hasFinished will return false approximately 50 times before
// returning true. For bigger sleep values and a large number of
// objects, the impact of this is significant. A good solution would
// be to pool scheduled objects together and only perform one check on
// the entire pool. If the pool is due, all the nodes within it are
// inserted into the scheduler for detailed checking. We could have a
// series of such pools, ordered by expiration time, so that we only
// ever need to check the first pool in the list. The optimal pool
// interval, number of pools etc depends on the application and the
// fps - but it should be possible to find some reasonable defaults. A
// more generalized priority queue implementation is also possible.
class SleepManager
{
 private:
  // Instance of the timer class that is associated with this timer
  MonsterObject *tobj;

  // Current tick count
  long current;

 public:

  // Specify a Monster object to associate with this timer. Use 'null'
  // if you don't need an object.
  this(MonsterObject *obj)
    {
      if(obj is null) return;

      tobj = obj;
      tobj.getExtra(_timerClass).obj = this;
    }

  // By default, create a new object
  this()
    { this(_timerClass.createObject); }

  // Number of 'ticks' per second
  static const long tickSize = 1000;

  // Reset the timer to zero
  void reset() { current = 0; }

  // Return the total number of elapsed seconds since start (or last
  // reset)
  double read() { return current/cast(double)tickSize; }

  // Add time to the timer.
  void add(double d) { current += cast(long)(tickSize*d); }
  void addl(long l) { current += l; }

  MonsterObject *getObj() { return tobj; }
}

SleepManager idleTime;

MonsterClass _timerClass;

void initTimerModule()
{
  if(_timerClass !is null)
    {
      assert(idleTime !is null);
      return;
    }

  _timerClass = new MonsterClass(MC.String, moduleDef, "timer");

  assert(idleTime is null);
  idleTime = new SleepManager(_timerClass.getSing());

  setSleepMethod(SleepMethod.Clock);
}

enum SleepMethod
  { Clock, Timer }

void setSleepMethod(SleepMethod meth)
{
  initTimerModule();

  if(meth == SleepMethod.Clock)
    _timerClass.bind("sleep", new IdleSleep_SystemClock);
  else if(meth == SleepMethod.Timer)
    _timerClass.bind("sleep", new IdleSleep_Timer);
  else assert(0, "unknown timer method");
}
