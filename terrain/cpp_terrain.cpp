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

SceneNode *g_rootTerrainNode;
int g_alphaSize;

struct MeshInfo;
struct AlphaInfo;

// D functions
extern "C"
{
  void d_terr_superman();
  void d_terr_terrainUpdate();

  char *d_terr_getTexName(int32_t);

  void d_terr_fillVertexBuffer(const MeshInfo*,float*);
  void d_terr_fillIndexBuffer(const MeshInfo*,uint16_t*);
  AlphaInfo *d_terr_getAlphaInfo(const MeshInfo*,int32_t);

  void d_terr_fillAlphaBuffer(const AlphaInfo*,uint8_t*);
}

// Info about a submesh. This is a clone of the struct defined in
// archive.d. TODO: Make sure the D and C++ structs are of the same
// size and alignment.
struct MeshInfo
{
  // Bounding box info
  float minHeight, maxHeight;
  float worldWidth;

  // Vertex and index numbers
  int32_t vertRows, vertCols;
  int32_t indexCount;

  // Scene node position (relative to the parent node)
  float x, y;

  // Height offset to apply to all vertices
  float heightOffset;

  // Size and offset of the vertex buffer
  int64_t vertBufSize, vertBufOffset;

  // Number and offset of AlphaInfo blocks
  int32_t alphaNum;
  uint64_t alphaOffset;

  // Texture name. Index to the string table.
  int32_t texName;

  inline void fillVertexBuffer(float *buffer) const
  {
    d_terr_fillVertexBuffer(this, buffer);
  }

  inline void fillIndexBuffer(uint16_t *buffer) const
  {
    d_terr_fillIndexBuffer(this, buffer);
  }

  inline char* getTexName() const
  {
    return d_terr_getTexName(texName);
  }

  inline AlphaInfo *getAlphaInfo(int tnum) const
  {
    return d_terr_getAlphaInfo(this, tnum);
  }
};

// Info about an alpha map belonging to a mesh
struct AlphaInfo
{
  // Position of the actual image data
  uint64_t bufSize, bufOffset;

  // The texture name for this layer. The actual string is stored in
  // the archive's string buffer.
  int32_t texName;
  int32_t alphaName;

  inline char* getTexName() const
  {
    return d_terr_getTexName(texName);
  }

  inline char* getAlphaName() const
  {
    return d_terr_getTexName(alphaName);
  }

  inline void fillAlphaBuffer(uint8_t *buffer) const
  {
    return d_terr_fillAlphaBuffer(this, buffer);
  }
};

#include "cpp_baseland.cpp"
#include "cpp_mesh.cpp"

BaseLand *g_baseLand;

class TerrainFrameListener : public FrameListener
{
protected:
  bool frameEnded(const FrameEvent& evt)
  {
    d_terr_terrainUpdate();
    g_baseLand->update();
    return true;
  }
};

// Functions called from D
extern "C"
{
  SceneNode* terr_createChildNode(float x, float y,
                                  SceneNode *parent)
  {
    Ogre::Vector3 pos(x,y,0);
    if(parent == NULL)
      parent = g_rootTerrainNode;

    assert(parent);
    return parent->createChildSceneNode(pos);
  }

  void terr_destroyNode(SceneNode *node)
  {
    node->removeAndDestroyAllChildren();
    mSceneMgr->destroySceneNode(node);
  }

  // TODO: We could make allocation a little more refined than new and
  // delete. But that's true for everything here. A freelist based
  // approach is best in most of these cases, as we have continuous
  // allocation/deallocation of fixed-size structs.
  Ogre::AxisAlignedBox *terr_makeBounds(float minHeight, float maxHeight,
                                        float width, SceneNode* node)
  {
    AxisAlignedBox *mBounds = new AxisAlignedBox;

    mBounds->setExtents(0,0,minHeight,
                        width,width,maxHeight);

    // Transform the box to world coordinates, so it can be compared
    // with the camera later.
    mBounds->transformAffine(node->_getFullTransform());

    return mBounds;
  }

  void terr_killBounds(AxisAlignedBox *bounds)
  {
    delete bounds;
  }

  float terr_getSqCamDist(AxisAlignedBox *mBounds)
  {
    Ogre::Vector3 cpos = mCamera->getDerivedPosition();
    Ogre::Vector3 diff(0, 0, 0);
    diff.makeFloor(cpos - mBounds->getMinimum() );
    diff.makeCeil(cpos - mBounds->getMaximum() );
    return diff.squaredLength();
  }

  TerrainMesh *terr_makeMesh(SceneNode *parent,
                             MeshInfo *info,
                             int level, float scale)
  {
    
    return new TerrainMesh(parent, *info, level, scale);
  }

  void terr_killMesh(TerrainMesh *mesh)
  { delete mesh; }

  // Set up the rendering system
  void terr_setupRendering()
  {
    // Make sure the C++ sizes match the D sizes, since the structs
    // will be shared between the two.
    assert(sizeof(MeshInfo) == 17*4);
    assert(sizeof(AlphaInfo) == 6*4);

    // Add the terrain directory as a resource location. TODO: Get the
    // name from D.
    ResourceGroupManager::getSingleton().
      addResourceLocation("cache/terrain/", "FileSystem", "General");

    // Enter superman mode
    mCamera->setFarClipDistance(32*CELL_WIDTH);
    //ogre_setFog(0.7, 0.7, 0.7, 200, 32*CELL_WIDTH);
    d_terr_superman();

    // Create a root scene node first. The 'root' node is rotated to
    // match the MW coordinate system
    g_rootTerrainNode = mwRoot->createChildSceneNode("TERRAIN_ROOT");

    // Add the base land. This is the ground beneath the actual
    // terrain mesh that makes the terrain look infinite.
    g_baseLand = new BaseLand();

    // Add the frame listener
    mRoot->addFrameListener(new TerrainFrameListener);
  }
}
