/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (player.d) is part of the OpenMW package.

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

module scene.player;

import ogre.ogre;
import monster.monster;

/*
 * Contains essential data about the player and other current game
 * data. This will be moved to script code.
 */

PlayerData playerData;

struct PlayerData
{
  MonsterObject *mo;

  Placement *position;
  int *level;

  // Set up the player object. This is still pretty hackish and
  // temporary.
  void setup()
  {
    assert(mo is null);
    mo = vm.load("game.player").getSing;
    level = mo.getIntPtr("level");

    // Still an ugly hack
    position = cast(Placement*)mo.getFloatPtr("x");
  }

  /*
  char[] getName()
  {
    assert(mo !is null);
    return mo.getString8("name");
  }
  */

  // Temp. way of selecting start point - used in celldata
  bool posSet = false;
}
