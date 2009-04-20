/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (init.d) is part of the Monster script language package.

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

// This module makes sure that the library is initialized in all
// cases.
module monster.vm.init;

import monster.compiler.tokenizer;
import monster.compiler.properties;
import monster.compiler.scopes;
import monster.vm.thread;
import monster.vm.stack;
import monster.vm.arrays;
import monster.vm.vm;

import monster.modules.all;
import monster.options;

// D runtime stuff
version(Posix)
{
  extern (C) void _STI_monitor_staticctor();
  //extern (C) void _STD_monitor_staticdtor();
  extern (C) void _STI_critical_init();
  //extern (C) void _STD_critical_term();
}
version(Win32)
{
  extern (C) void _minit();
}
extern (C) void gc_init();
//extern (C) void gc_term();
extern (C) void _moduleCtor();
//extern (C) void _moduleDtor();
extern (C) void _moduleUnitTests();

//extern (C) bool no_catch_exceptions;

bool initHasRun = false;

bool stHasRun = false;

static this()
{
  assert(!stHasRun);
  stHasRun = true;

  // While we're here, run the initializer right away if it hasn't run
  // already.
  if(!initHasRun)
    doMonsterInit();
}

void doMonsterInit()
{
  // Prevent recursion
  assert(!initHasRun, "doMonsterInit should never run more than once");
  initHasRun = true;

  // First check if D has been initialized.
  if(!stHasRun)
    {
      // Nope. This is normal though if we're running as a C++
      // library. We have to init the D runtime manually.

      version (Posix)
        {
          _STI_monitor_staticctor();
          _STI_critical_init();
        }

      gc_init();

      version (Win32)
        {
          _minit();
        }

      _moduleCtor();
      _moduleUnitTests();
    }

  assert(stHasRun, "D library initializion failed");

  // Next, initialize the Monster library

  // Initialize compiler constructs
  initTokenizer();
  initProperties();
  initScope();

  // Initialize VM
  vm.doVMInit();
  scheduler.init();
  stack.init();
  arrays.initialize();

  // Load modules
  static if(loadModules)
    initAllModules();
}
