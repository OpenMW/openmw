/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Jacob Essex, Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (cpp_terrain.cpp) is part of the OpenMW package.

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

///no texture assigned
const int LAND_LTEX_NONE = 0;

///the default land height that it defaults to in the TESCS
const int LAND_DEFAULT_HEIGHT = -2048;

///how many verts wide (and long) the cell is
const int LAND_VERT_WIDTH = 65;

///Number of verts that make up a cell
const int LAND_NUM_VERTS = LAND_VERT_WIDTH*LAND_VERT_WIDTH;

const int LAND_LTEX_WIDTH = 16;
const int LAND_NUM_LTEX = LAND_LTEX_WIDTH*LAND_LTEX_WIDTH;

const int CELL_WIDTH = 8192;

// Scaling factor to apply to textures on once cell. A factor of 1/16
// gives one repetition per square, since there are 16x16 texture
// 'squares' in acell. For reference, Yacoby's scaling was equivalent
// to having 1.0/10 here, or 10 repititions per cell. TODO: This looks
// a little blocky. Compare with screenshots from TES-CS.
const float TEX_SCALE = 1.0/16;

// Multiplied with the size of the quad. If these are too close, a
// quad might end up splitting/unsplitting continuously, since the
// quad size changes when we split.
const float SPLIT_FACTOR = 0.5;
const float UNSPLIT_FACTOR = 2.0;

//stops it crashing, now it leaks.
#define ENABLED_CRASHING 0

class Quad;
class TerrainMesh;
class BaseLand;

// Cache directory and file
std::string g_cacheDir;
std::string g_cacheFile;

// Enable or disable tracing of runtime functions. Making RTRACE do a
// trace slows down the code significantly even when -debug is off, so
// lets disable it for normal use.
#define RTRACE(x)
//#define RTRACE TRACE

/*
// Prerequisites
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>

// Located in ../util/
#include "mmfile.h"
#include "outbuffer.h"

// Reading and writing the cache files
#include "cpp_archive.cpp"
#include "cpp_cachewriter.cpp"

// For generation
#include "cpp_esm.cpp"
#include "cpp_landdata.cpp"
#include "cpp_generator.cpp"

// For rendering
Quad *g_rootQuad;
BaseLand *g_baseLand;
SceneNode *g_rootTerrainNode;

#include "cpp_baseland.cpp"
#include "cpp_mesh.cpp"
#include "cpp_quad.cpp"

class TerrainFrameListener : public FrameListener
{
protected:
  bool frameEnded(const FrameEvent& evt)
  {
    g_rootQuad->update();
    g_baseLand->update();
    return true;
  }
};

extern "C" void d_superman();
*/

extern "C" void terr_setCacheDir(char *cacheDir)
{
  g_cacheDir = cacheDir;
  g_cacheFile = g_cacheDir + "terrain.cache";
}

// Set up the rendering system
extern "C" void terr_setupRendering()
{
  /*
  // Add the terrain directory
  ResourceGroupManager::getSingleton().
    addResourceLocation(g_cacheDir, "FileSystem", "General");

  // Create a root scene node first. The 'root' node is rotated to
  // match the MW coordinate system
  g_rootTerrainNode = root->createChildSceneNode("TERRAIN_ROOT");

  // Open the archive file
  g_archive.openFile(g_cacheFile);

  // Create the root quad.
  g_rootQuad = new Quad();

  g_baseLand = new BaseLand(g_rootTerrainNode);

  // Add the frame listener
  mRoot->addFrameListener(new TerrainFrameListener);

  // Enter superman mode
  mCamera->setFarClipDistance(32*CELL_WIDTH);
  //ogre_setFog(0.7, 0.7, 0.7, 200, 32*CELL_WIDTH);
  d_superman();
  */
}

/*
// Generate all cached data.
extern "C" void terr_genData()
{ 
  Ogre::Root::getSingleton().renderOneFrame();

  Generator mhm;
  {
    ESM esm;

    const std::string fn("data/Morrowind.esm");

    esm.addRecordType("LAND", "INTV");
    esm.addRecordType("LTEX", "INTV");

    esm.loadFile(fn);
    RecordList* land = esm.getRecordsByType("LAND");
    for ( RecordListItr itr = land->begin(); itr != land->end(); ++itr )
      mhm.addLandData(*itr, fn);

    RecordList* ltex = esm.getRecordsByType("LTEX");
    for ( RecordListItr itr = ltex->begin(); itr != ltex->end(); ++itr )
      mhm.addLandTextureData(*itr, fn);
  }

  mhm.generate(g_cacheFile);
}
*/
