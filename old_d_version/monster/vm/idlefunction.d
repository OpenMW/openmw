/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (idlefunction.d) is part of the Monster script language
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

module monster.vm.idlefunction;

import monster.vm.thread;

// Idle scheduling actions - returned from initiate()
enum IS
  {
    Poll,    // Poll the hasFinished function regularily
    Return,  // Return to the thread immediately - reenter() is called
             // first
    Kill,    // Kill the thread
    Manual,  // Handle the scheduling ourselves. We have to schedule
             // or move the thread to another ThreadList before the
             // end of initiate()
  }

// A callback class for idle functions. A child object of this class
// is what you "bind" to idle functions (rather than just a delegate,
// like for native functions.) Note that instances are not bound to
// specific script objects or threads; one idle function instance may
// be called for many objects / threads simultaneously. Any data
// specific to this call (such as parameters) must be stored
// elsewhere, usually within the Thread.
abstract class IdleFunction
{
  // This is called immediately after the idle function is "called"
  // from M script. It has to handle function parameters (remove them
  // from the stack), but otherwise does not have to do
  // anything. Return true if the scheduler should put this idle
  // function into the condition list, which is usually a good
  // idea. For functions which never "return", and for event driven
  // idle functions (which handle their own scheduling), you should
  // return false.
  abstract IS initiate(Thread*);

  // This is called whenever the idle function is about to "return" to
  // state code. It has to push the return value, if any, but
  // otherwise it can be empty. Note that if the idle function is
  // aborted (eg. the state is changed), this function is never
  // called, and abort() is called instead.
  void reentry(Thread*) {}

  // Called whenever an idle function is aborted, for example by a
  // state change. No action is usually required.
  void abort(Thread*) {}

  // The condition that determines if this function has finished. This
  // is the main method by which the scheduler determines when to
  // reenter M state code. For example, for an idle function
  // waitSoundFinish(), this would return false if the sound is still
  // playing, and true if the sound has finished. If you want a purely
  // event-driven idle function (rather than polling each frame), you
  // should return false in initiate and instead reschedule the object
  // manually when the event occurs. (A nice interface for this has
  // not been created yet, though.)
  bool hasFinished(Thread*) { assert(0, "empty hasFinished()"); }
}
