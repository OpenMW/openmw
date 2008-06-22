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

/*
 * Contains essential data about the player and other current game
 * data. This will be moved to script code.
 */

PlayerData playerData;

struct PlayerData
{
  // Position and rotation. The rotation is not updated continuously,
  // only the position.
  Placement position;

  // Just an example value, we use it for resolving leveled lists
  // (lists that determine eg. what creatures to put in a cave based
  // on what level the player is.)
  short level = 5;

  // Temp. way of selecting start point - used in celldata
  bool posSet = false;
}
