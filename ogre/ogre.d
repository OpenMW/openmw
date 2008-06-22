/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (ogre.d) is part of the OpenMW package.

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

module ogre.ogre;

import core.resource;

import ogre.bindings;
import util.uniquename;
import std.stdio;

import esm.defs;

class OgreException : Exception
{
  this(char[] msg) {super("OgreException: " ~ msg);}

  static void opCall(char[] msg)
    {
      throw new OgreException(msg);
    }
}

// Place a mesh in the 3D scene graph, at the given
// location/scale. Returns a node pointer to the inserted object.
NodePtr placeObject(MeshIndex mesh, Placement *pos, float scale)
{
  // Get a scene node for this object. mesh.getNode() will either load
  // it from file or BSA archive, or give us a handle if it is already
  // loaded.

  // This must be called BEFORE UniqueName below, because it might
  // possibly use UniqueName itself and overwrite the data
  // there. (That was a fun bug to track down...)
  NodePtr node = mesh.getNode();

  // Let us insert a copy
  char[] name = UniqueName(mesh.getName);
  return cpp_insertNode(node, name.ptr, pos, scale);
}

NodePtr attachLight(NodePtr parent, Color c, float radius)
{
  return cpp_attachLight(UniqueName("_light").ptr, parent,
			 c.red/255.0, c.green/255.0, c.blue/255.0,
			 radius);
}

// If 'true' then we must call cpp_cleanup() on exit.
bool ogreSetup = false;

// Make sure we clean up
static ~this()
{
  cleanupOgre();
}

// Loads ogre configurations, creats the root, etc.
void setupOgre()
{
  // Later we will send some config info from core.config along with
  // this function
  if(cpp_configure()) OgreException("Configuration abort");

  cpp_initWindow();

  // We set up the scene manager in a separate function, since we
  // might have to do that for every new cell later on, and handle
  // exterior cells differently, etc.
  cpp_makeScene();

  ogreSetup = true;
}

void setAmbient(Color amb, Color sun, Color fog, float density)
{
  cpp_setAmbient(amb.red/255.0, amb.green/255.0, amb.blue/255.0,
		 sun.red/255.0, sun.green/255.0, sun.blue/255.0);

  // Calculate fog distance
  // TODO: Mesh with absolute view distance later
  float fhigh = 4500 + 9000*(1-density);
  float flow = 200 + 2000*(1-density);

  cpp_setFog(fog.red/255.0, fog.green/255.0, fog.blue/255.0, 200, fhigh);
}

// Jump into the OGRE rendering loop. Everything should be loaded and
// done before calling this.
void startRendering()
{
  // Kick OGRE into gear
  cpp_startRendering();
}

// Cleans up after OGRE. Resets things like screen resolution and
// mouse control.
void cleanupOgre()
{
  if(ogreSetup)
    cpp_cleanup();

  ogreSetup = false;
}

// Gives the placement of an item in the scene (position and
// orientation). It must have this exact structure since we also use
// it when reading ES files.
align(1) struct Placement
{
  float[3] position;
  float[3] rotation;
}
