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
import ogre.bindings;

import std.string;

// Set up the base Monster classes we need in OpenMW
void initMonsterScripts()
{
  initAllModules();
  setSleepMethod(SleepMethod.Timer);

  // Add the script directories
  vm.addPath("mscripts/");
  vm.addPath("mscripts/gameobjects/");
  vm.addPath("mscripts/sound/");

  // Import some modules into the global scope, so we won't have to
  // import them manually in each script.
  global.registerImport("io", "random", "timer");

  // Get the Config singleton object
  config.mo = (new MonsterClass("Config")).getSing();

  // Run the fps ticker
  //vm.run("fpsticker.mn");
  auto mc = new MonsterClass("fpsticker.mn");
  mc.bind("setText", &setFpsText);
  mc.createObject().call("run");

  // Run the test script
  vm.run("test.mn");
}

void setFpsText()
{
  AIndex[] args = stack.popAArray();

  char[] res;

  foreach(AIndex ind; args)
    res ~= format("%s", arrays.getRef(ind).carr);

  gui_setFpsText(res.ptr);
}
