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

const int CELL_WIDTH = 8192;

#include "cpp_baseland.cpp"
//#include "cpp_mesh.cpp"

BaseLand *g_baseLand;
SceneNode *g_rootTerrainNode;

class TerrainFrameListener : public FrameListener
{
protected:
  bool frameEnded(const FrameEvent& evt)
  {
    //g_rootQuad->update();
    g_baseLand->update();
    return true;
  }
};

extern "C"
{
  void d_superman();

  SceneNode* terr_createChildNode(float relX, float relY,
                                  SceneNode *parent)
  {}

  void terr_destroyNode(SceneNode *node)
  {}

  void *terr_makeBounds(float minHeight, float maxHeight,
                        float width)
  {}

  float terr_getSqCamDist(void*)
  {}

  void *terr_makeMesh(int segment, SceneNode*)
  {}

  void terr_killMesh(void*)
  {}

  // Set up the rendering system
  void terr_setupRendering()
  {
    // Create a root scene node first. The 'root' node is rotated to
    // match the MW coordinate system
    g_rootTerrainNode = root->createChildSceneNode("TERRAIN_ROOT");

    // Add the base land. This is the ground beneath the actual
    // terrain mesh that makes the terrain look infinite.
    g_baseLand = new BaseLand(g_rootTerrainNode);

    /*
    // Add the terrain directory
    ResourceGroupManager::getSingleton().
      addResourceLocation(g_cacheDir, "FileSystem", "General");

    // Open the archive file
    g_archive.openFile(g_cacheFile);

    // Create the root quad.
    g_rootQuad = new Quad();
    */

    // Add the frame listener
    mRoot->addFrameListener(new TerrainFrameListener);

    // Enter superman mode
    mCamera->setFarClipDistance(32*CELL_WIDTH);
    //ogre_setFog(0.7, 0.7, 0.7, 200, 32*CELL_WIDTH);
    d_superman();
  }
}
