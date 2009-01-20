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

module mscripts.object;

import monster.monster;
import monster.modules.timer;

import std.stdio;
import std.date;
import core.resource : rnd;
import core.config;
import sound.music;

// Set up the base Monster classes we need in OpenMW
void initMonsterScripts()
{
  initAllModules();
  setSleepMethod(SleepMethod.Timer);

  // Add the script directories
  vm.addPath("mscripts/");
  vm.addPath("mscripts/gameobjects/");
  vm.addPath("mscripts/sound/");

  // Make sure the Object class is loaded
  auto mc = new MonsterClass("Object", "object.mn");

  // Get the Config singleton object
  config.mo = (new MonsterClass("Config")).getSing();

  // Bind various functions
  mc.bind("randInt",
  { stack.pushInt(rnd.randInt
    (stack.popInt,stack.popInt));});

  // Run the fps ticker
  vm.run("fpsticker.mn");

  // Run the test script
  vm.run("test.mn");
}
