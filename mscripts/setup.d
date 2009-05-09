/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (object.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
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

module mscripts.setup;

import monster.monster;
import monster.compiler.scopes : global;
import monster.modules.timer;

import core.config;
import ogre.gui;

import std.string;

// Set up the base Monster classes we need in OpenMW
void initMonsterScripts()
{
  // Add the script directories
  vm.addPath("mscripts/");

  // Import some modules into the global scope, so we won't have to
  // import them manually in each script.
  global.registerImport("io", "random", "timer");

  // Get the Config singleton object
  config.mo = vm.load("Config").getSing();

  // Set up the GUI Monster module
  setupGUIScripts();
}

// Run the GUI scripts. These should be run only after the
// GUI/rendering system has been initialized
void runGUIScripts()
{
  // Create the HUD and windows
  vm.run("makegui.mn");

  // Run the fps ticker
  vm.run("fpsticker.mn");
}

// This should probably not be here:
import monster.vm.dbg;
import std.string;

extern(C):
void dbg_trace(char*str) { dbg.trace(toString(str)); }
void dbg_untrace() { dbg.untrace(); }
