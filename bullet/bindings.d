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

typedef void* BulletShape;

extern(C):

// Initialize the dynamic world. Returns non-zero if an error occurs.
int bullet_init();

// Switch to the next physics mode
void bullet_nextMode();

// Warp the player to a specific location.
void bullet_movePlayer(float x, float y, float z);

// Request that the player moves in this direction
void bullet_setPlayerDir(float x, float y, float z);

// Get the current player position, after physics and collision have
// been applied.
void bullet_getPlayerPos(float *x, float *y, float *z);

// Create a box shape.
/*
void bullet_createBoxShape(float minX, float minY, float minZ,
                           float maxX, float maxY, float maxZ,
                           float *trans,float *matrix);
*/

// Create a triangle shape. This is cumulative, all meshes created
// with this function are added to the same shape. Since the various
// parts of a mesh can be differently transformed and we are putting
// them all in one shape, we must transform the vertices manually.
void bullet_createTriShape(int numFaces,
                           void *triArray,
                           int numVerts,
                           void *vertArray,
                           float *trans,float *matrix);

// "Flushes" the meshes created with createTriShape, returning the
// pointer to the final shape object.
BulletShape bullet_getFinalShape();

// Insert a static mesh with the given translation, quaternion
// rotation and scale. The quaternion is assumed to be in Ogre format,
// ie. with the W first.
void bullet_insertStatic(BulletShape shp, float *pos,
                         float *quat, float scale);

// Move the physics simulation 'delta' seconds forward in time
void bullet_timeStep(float delta);

// Deallocate objects
void bullet_cleanup();

