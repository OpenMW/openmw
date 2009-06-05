/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
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
import core.config;

import ogre.bindings;
import gui.bindings;
import mscripts.setup;

import bullet.bindings;
import util.uniquename;
import std.stdio;

import esm.defs;


extern(C) {
extern int lightConst;
extern float lightConstValue;

extern int lightLinear;
extern int lightLinearMethod;
extern float lightLinearValue;
extern float lightLinearRadiusMult;

extern int lightQuadratic;
extern int lightQuadraticMethod;
extern float lightQuadraticValue;
extern float lightQuadraticRadiusMult;

extern int lightOutQuadInLin;
}


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
NodePtr placeObject(MeshIndex mesh, Placement *pos, float scale,
                    bool collide)
{
  // Get a scene node for this object. mesh.getNode() will either load
  // it from file or BSA archive, or give us a handle if it is already
  // loaded.

  // This must be called BEFORE UniqueName below, because it might
  // possibly use UniqueName itself and overwrite the data
  // there. (That was a fun bug to track down...) Calling getNode()
  // will load the mesh if it is not already loaded.
  NodePtr node = mesh.getNode();

  // First, convert the Morrowind rotation to a quaternion
  float[4] quat;
  ogre_mwToQuaternion(pos.rotation.ptr, quat.ptr);

  // Insert a mesh copy into Ogre.
  char[] name = UniqueName(mesh.getName);
  node = ogre_insertNode(node, name.ptr, pos.position.ptr,
                         quat.ptr, scale);

  // Insert a collision shape too, if the mesh has one.
  if(collide && mesh.shape !is null)
    bullet_insertStatic(mesh.shape, pos.position.ptr,
                        quat.ptr, scale);

  return node;
}

NodePtr attachLight(NodePtr parent, Color c, float radius)
{
  return ogre_attachLight(UniqueName("_light").ptr, parent,
			 c.red/255.0, c.green/255.0, c.blue/255.0,
			 radius);
}

// If 'true' then we must call ogre_cleanup() on exit.
bool ogreSetup = false;

// Make sure we clean up
static ~this()
{
  cleanupOgre();
}

// Loads ogre configurations, creats the root, etc.
void setupOgre(bool debugOut)
{
  char[] plugincfg;

  version(Windows)
    plugincfg = "plugins.cfg.win32";
  else version(Posix)
    plugincfg = "plugins.cfg.linux";
  else
    // Assume the user knows what to do
    plugincfg = "plugins.cfg";

  // Later we will send more config info from core.config along with
  // this function
  if(ogre_configure(config.finalOgreConfig, toStringz(plugincfg), debugOut))
    OgreException("Configuration abort");

  ogre_initWindow();

  // We set up the scene manager in a separate function, since we
  // might have to do that for every new cell later on, and handle
  // exterior cells differently, etc.
  ogre_makeScene();

  ogreSetup = true;
}

void setAmbient(Color amb, Color sun, Color fog, float density)
{
  ogre_setAmbient(amb.red/255.0, amb.green/255.0, amb.blue/255.0,
		 sun.red/255.0, sun.green/255.0, sun.blue/255.0);

  // Calculate fog distance
  // TODO: Mesh with absolute view distance later
  float fhigh = 4500 + 9000*(1-density);
  float flow = 200 + 2000*(1-density);

  ogre_setFog(fog.red/255.0, fog.green/255.0, fog.blue/255.0, 200, fhigh);
}

// Jump into the OGRE rendering loop. Everything should be loaded and
// done before calling this.
void startRendering()
{
  // Kick OGRE into gear
  ogre_startRendering();
}

// Cleans up after OGRE. Resets things like screen resolution, mouse
// control and keyboard repeat rate.
void cleanupOgre()
{
  if(ogreSetup)
    ogre_cleanup();

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
static assert(Placement.sizeof == 4*6);
