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

// Shader names
#define MORPH_VERTEX_PROGRAM "mw_terrain_VS"
#define FADE_FRAGMENT_PROGRAM "mw_terrain_texfade_FP"

// Directories
#define TEXTURE_OUTPUT "cache/terrain/"
#define TERRAIN_OUTPUT "cache/terrain/landscape"

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

// Multiplied with the size of the quad. If these are too close, a
// quad might end up splitting/unsplitting continuously, since the
// quad size changes when we split.
const float SPLIT_FACTOR = 0.5;
const float UNSPLIT_FACTOR = 2.0;

//stops it crashing, now it leaks.
#define ENABLED_CRASHING 0

class Quad;
class QuadData;
class Terrain;
class MaterialGenerator;
class MWHeightmap;

MWHeightmap *g_heightMap;
MaterialGenerator *g_materialGen;
Terrain *g_Terrain;

#undef TRACE
#define TRACE(x)

// Prerequisites
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

// For generation
#include "cpp_materialgen.cpp"
#include "cpp_esm.cpp"
#include "cpp_landdata.cpp"
#include "cpp_quaddata.cpp"
#include "cpp_index.cpp"
#include "cpp_palette.cpp"
#include "cpp_point2.cpp"
#include "cpp_generator.cpp"

// For rendering
#include "cpp_baseland.cpp"
#include "cpp_mwheightmap.cpp"

// These depend on each other, so our usual hackery won't work. We
// need the header files first.
#include "cpp_terrainmesh.h"
#include "cpp_terraincls.h"

#include "cpp_quad.cpp"

#include "cpp_terraincls.cpp"
#include "cpp_terrainmesh.cpp"

#include "cpp_framelistener.cpp"

TerrainFrameListener terrainListener;

extern "C" void d_superman();

// Set up the rendering system
extern "C" void terr_setupRendering()
{
  if(!g_materialGen)
    g_materialGen = new MaterialGenerator;

  // Add the terrain directory
  ResourceGroupManager::getSingleton().
    addResourceLocation(TEXTURE_OUTPUT, "FileSystem", "General");

  // Set up the terrain frame listener
  terrainListener.setup();

  // Enter superman mode
  mCamera->setFarClipDistance(32*8192);
  d_superman();
}

// Generate all cached data.
extern "C" void terr_genData()
{ 
  if(!g_materialGen)
    g_materialGen = new MaterialGenerator;

  Ogre::Root::getSingleton().renderOneFrame();

  Generator mhm(TERRAIN_OUTPUT);
  {
    ESM esm;

    const std::string fn("data/Morrowind.esm");

    esm.addRecordType("LAND", "INTV");
    esm.addRecordType("LTEX", "INTV");

    esm.loadFile(fn);
    RecordListPtr land = esm.getRecordsByType("LAND");
    for ( RecordListItr itr = land->begin(); itr != land->end(); ++itr )
      mhm.addLandData(*itr, fn);

    RecordListPtr ltex = esm.getRecordsByType("LTEX");
    for ( RecordListItr itr = ltex->begin(); itr != ltex->end(); ++itr )
      mhm.addLandTextureData(*itr, fn);
  }

  mhm.beginGeneration();

  mhm.generateLODLevel(6, true, 1024);
  mhm.generateLODLevel(5, true, 512);
  mhm.generateLODLevel(4, true, 256);
  mhm.generateLODLevel(3, true, 256);
  mhm.generateLODLevel(2, true, 256);
  mhm.generateLODLevel(1, false, 128);

  mhm.endGeneration();
}
