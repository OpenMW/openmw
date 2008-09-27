/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bindings.d) is part of the OpenMW package.

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

module bullet.bindings;

/*
 * This module is the interface between D and the C++ code that
 * handles Bullet.
 */

extern(C):

// Initialize the dynamic world. Returns non-zero if an error occurs.
int cpp_initBullet();
void cpp_timeStep(float delta);
void cpp_cleanupBullet();

// Insert a debug collision object
void cpp_insertBox(float x, float y, float z);

// Move the player's collision shape
int cpp_movePlayerCollision(float x, float y, float z,
                            float dx, float dy, float dz);
