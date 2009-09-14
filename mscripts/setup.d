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
import gui.gui;
import scene.player;

import std.string;
import std.stdio;

// Set up the base Monster classes we need in OpenMW
void initMonsterScripts()
{
  // Add the script directories
  vm.addPath("mscripts/");

  // Import some modules into scope of Object, so we won't have to
  // import them manually into each script.
  MonsterClass.getObject().sc.registerImport("random", "timer");

  // Get the Config singleton object
  config.mo = vm.load("Config").getSing();

  // Set up the player object.
  playerData.setup();

  // Set up the GUI Monster module
  setupGUIScripts();

  // Add test cases for scripts here. This allows them to be tested
  // without engaging the rendering engine (specify the -n command
  // line switch.)
}

// This should probably not be here:
import monster.vm.dbg;

extern(C):
void dbg_trace(char*str) { dbg.trace(toString(str)); }
void dbg_untrace() { dbg.untrace(); }
