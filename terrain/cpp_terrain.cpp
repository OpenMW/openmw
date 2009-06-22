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

  void d_terr_fillVertexBuffer(const MeshInfo*,float*,uint64_t);
  void d_terr_fillIndexBuffer(const MeshInfo*,uint16_t*,uint64_t);
  AlphaInfo *d_terr_getAlphaInfo(const MeshInfo*,int32_t);

  void d_terr_fillAlphaBuffer(const AlphaInfo*,uint8_t*,uint64_t);

  int32_t d_terr_getAlphaSize();
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

  // Height offset to apply to all vertices
  float heightOffset;

  // Size and offset of the vertex buffer
  int64_t vertBufSize, vertBufOffset;

  // Number and offset of AlphaInfo blocks
  int32_t alphaNum;
  uint64_t alphaOffset;

  // Texture name. Index to the string table.
  int32_t texName;

  inline void fillVertexBuffer(float *buffer, uint64_t size) const
  {
    d_terr_fillVertexBuffer(this, buffer, size);
  }

  inline void fillIndexBuffer(uint16_t *buffer, uint64_t size) const
  {
    d_terr_fillIndexBuffer(this, buffer, size);
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

  inline void fillAlphaBuffer(uint8_t *buffer, uint64_t size) const
  {
    return d_terr_fillAlphaBuffer(this, buffer, size);
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
    TRACE("Terrain frame");
    d_terr_terrainUpdate();
    g_baseLand->update();
    return true;
  }
};

// Renders a material into a texture
Ogre::TexturePtr getRenderedTexture(Ogre::MaterialPtr mp,
                                    const std::string& name,
                                    int texSize, Ogre::PixelFormat tt)
{
  Ogre::CompositorPtr cp = Ogre::CompositorManager::getSingleton().
    create("Rtt_Comp",
           Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

  Ogre::CompositionTargetPass* ctp = cp->createTechnique()->getOutputTargetPass();
  Ogre::CompositionPass* cpass = ctp->createPass();
  cpass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
  cpass->setMaterial(mp);

  // Create the destination texture
  Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().
    createManual(name + "_T",
                 Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                 Ogre::TEX_TYPE_2D,
                 texSize,
                 texSize,
                 0,
                 tt,
                 Ogre::TU_RENDERTARGET
                 );

  Ogre::RenderTexture* renderTexture = texture->getBuffer()->getRenderTarget();
  Ogre::Viewport* vp = renderTexture->addViewport(mCamera);

  Ogre::CompositorManager::getSingleton().addCompositor(vp, "Rtt_Comp");
  Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp,"Rtt_Comp", true);

  renderTexture->update();

  // Call the OGRE renderer.
  Ogre::Root::getSingleton().renderOneFrame();

  Ogre::CompositorManager::getSingleton().removeCompositor(vp, "Rtt_Comp");
  Ogre::CompositorManager::getSingleton().remove(cp->getHandle());

  renderTexture->removeAllViewports();

  return texture;
}

// These are used between some functions below. Kinda messy. Since
// these are GLOBAL instances, they are terminated at program
// exit. However, OGRE itself is terminated before that, so we have to
// make sure we have no 'active' shared pointers after OGRE is
// finished (otherwise we get a segfault at exit.)
std::list<Ogre::ResourcePtr> createdResources;
Ogre::HardwarePixelBuffer *pixelBuffer;
MaterialPtr mat;

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
    TRACE("terr_makeBounds");
    AxisAlignedBox *mBounds = new AxisAlignedBox;

    assert(maxHeight >= minHeight);

    mBounds->setExtents(0,0,minHeight,
                        width,width,maxHeight);

    // Transform the box to world coordinates, so it can be compared
    // with the camera later.
    mBounds->transformAffine(node->_getFullTransform());

    return mBounds;
  }

  void terr_killBounds(AxisAlignedBox *bounds)
  {
    TRACE("terr_killBounds");
    delete bounds;
  }

  float terr_getSqCamDist(AxisAlignedBox *mBounds)
  {
    TRACE("terr_getSqCamDist");
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
  {
    TRACE("terr_killMesh");
    delete mesh;
  }

  // Set up the rendering system
  void terr_setupRendering()
  {
    TRACE("terr_setupRendering()");
    // Make sure the C++ sizes match the D sizes, since the structs
    // are shared between the two.
    assert(sizeof(MeshInfo) == 14*4);
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

    g_alphaSize = d_terr_getAlphaSize();

    // Add the frame listener
    mRoot->addFrameListener(new TerrainFrameListener);
  }

  // The next four functions are called in the function genLevel2Map()
  // only. This is very top-down-programming-ish and a bit messy, but
  // that's what I get for mixing C++ and D like this.
  void terr_makeLandMaterial(const char* name, float scale)
  {
    // Get a new material
    mat = Ogre::MaterialManager::getSingleton().
      create(name,
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    // Put the default texture in the bottom 'layer', so that we don't
    // end up seeing through the landscape.
    Ogre::Pass* np = mat->getTechnique(0)->getPass(0);
    np->setLightingEnabled(false);
    np->createTextureUnitState("_land_default.dds")
      ->setTextureScale(scale,scale);    
  }

  uint8_t *terr_makeAlphaLayer(const char* name, int32_t width)
  {
    // Create alpha map for this texture.
    Ogre::TexturePtr texPtr = Ogre::TextureManager::getSingleton().
      createManual(name,
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                   Ogre::TEX_TYPE_2D,
                   width, width,
                   1,0, // depth, mipmaps
                   Ogre::PF_A8, // One-channel alpha
                   Ogre::TU_STATIC_WRITE_ONLY);

    createdResources.push_back(texPtr);

    pixelBuffer = texPtr->getBuffer().get();
    pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
    const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

    return static_cast<Ogre::uint8*>(pixelBox.data);
  }

  void terr_closeAlpha(const char *alphaName,
                       const char *texName, float scale)
  {
    // Close the alpha pixel buffer opened in the previous function
    pixelBuffer->unlock();

    // Create a pass containing the alpha map
    Pass *np = mat->getTechnique(0)->createPass();
    np->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
    np->setLightingEnabled(false);
    np->setDepthFunction(Ogre::CMPF_EQUAL);
    Ogre::TextureUnitState* tus = np->createTextureUnitState(alphaName);
    tus->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

    // Set various blending options
    tus->setAlphaOperation(Ogre::LBX_BLEND_TEXTURE_ALPHA,
                           Ogre::LBS_TEXTURE,
                           Ogre::LBS_TEXTURE);
    tus->setColourOperationEx(Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                              Ogre::LBS_TEXTURE,
                              Ogre::LBS_TEXTURE);
    tus->setIsAlpha(true);

    // Add the terrain texture to the pass and scale it.
    tus = np->createTextureUnitState(texName);
    tus->setColourOperationEx(Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                              Ogre::LBS_TEXTURE,
                              Ogre::LBS_CURRENT);
    tus->setTextureScale(scale, scale);
  }

  // Clean up after the above functions, render the material to
  // texture and save the data in outdata and in the file outname.
  void terr_cleanupAlpha(const char *outname,
                         void *outData, int32_t toSize)
  {
    TexturePtr tex1 = getRenderedTexture(mat,outname,
                                         toSize,Ogre::PF_R8G8B8);

    // Blit the texture into the given memory buffer
    PixelBox pb = PixelBox(toSize, toSize, 1, PF_R8G8B8);
    pb.data = outData;
    tex1->getBuffer()->blitToMemory(pb);

    // Clean up
    TextureManager::getSingleton().remove(tex1->getHandle());
    const std::list<Ogre::ResourcePtr>::const_iterator iend = createdResources.end();
    for ( std::list<Ogre::ResourcePtr>::const_iterator itr = createdResources.begin();
          itr != iend;
          ++itr)
      (*itr)->getCreator()->remove((*itr)->getHandle());
    createdResources.clear();

    MaterialManager::getSingleton().remove(mat->getHandle());
    mat.setNull();
  }

  void terr_resize(void* srcPtr, void* dstPtr, int32_t fromW, int32_t toW)
  {
    // Create pixelboxes
    PixelBox src = PixelBox(fromW, fromW, 1, PF_R8G8B8);
    PixelBox dst = PixelBox(toW,   toW,   1, PF_R8G8B8);

    src.data = srcPtr;
    dst.data = dstPtr;

    // Resize the image. The nearest neighbour filter makes sure
    // there is no blurring.
    Image::scale(src, dst, Ogre::Image::FILTER_NEAREST);
  }

  void terr_saveImage(void *data, int32_t width, const char* name)
  {
    Image img;
    img.loadDynamicImage((uchar*)data, width, width, PF_R8G8B8);
    img.save(name);
  }
}
