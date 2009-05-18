class BaseLand
{
public:
  BaseLand(Ogre::SceneNode* s)
    : mTerrainSceneNode(s)
  {
    createMaterial();
    createMesh();
  }

  ~BaseLand()
  {
    destroyMaterial();
    destroyMesh();
  }

  // Repositions the mesh based on camera location
  void update()
  {
    Ogre::Real vd = mCamera->getFarClipDistance();
    // Recreate the mesh if the view distance has increased
    if ( vd > mMeshDistance  )
      {
        destroyMaterial();
        destroyMesh();
        createMaterial();
        createMesh();
      }

    Ogre::Vector3 p = mCamera->getDerivedPosition();
    p.x -= ((int)p.x % 8192);
    p.z -= ((int)p.z % 8192);

    float h = p.y + 2048;
    h = pow(h/8192*2,2);
    if ( h < 0 ) h = 0;

    mNode->setPosition(p.x, -32 - h, p.z);
  }

private:
  void createMesh()
  {
    mObject = mSceneMgr->createManualObject("BaseLand");
    mObject->begin("BaseLandMat", Ogre::RenderOperation::OT_TRIANGLE_LIST);

    Ogre::Real vd = mCamera->getFarClipDistance();
    vd += 8192 - ((int)vd % 8192);

    mMeshDistance = vd;

    mObject->position(-vd,-2048, vd);
    mObject->textureCoord(0, 1);

    mObject->position(vd,-2048, vd);
    mObject->textureCoord(1, 1);

    mObject->position(vd,-2048, -vd);
    mObject->textureCoord(1, 0);

    mObject->position(-vd,-2048, -vd);
    mObject->textureCoord(0, 0);

    mObject->quad(0,1,2,3);

    mObject->end();

    mNode = mTerrainSceneNode->createChildSceneNode();
    mNode->attachObject(mObject);
  }

  void destroyMesh()
  {
    mNode->detachAllObjects();
    mSceneMgr->destroyManualObject(mObject);
    mNode->getParentSceneNode()->removeAndDestroyChild(mNode->getName());
  }

  // FIXME: We destroy and recreate the material (and mesh) when the
  // view distance changes. If we make a built-in auto-adjusting FPS
  // optimizer, this will happen quite a lot, so it's worth trying to
  // optimize this. This might be moot however when we implement
  // water, since BaseLand may not be needed anymore.
  void createMaterial()
  {
    float vd = mCamera->getFarClipDistance();
    vd += 8192 - ((int)vd % 8192);
    vd = vd/8192 * 2;

    mMat = Ogre::MaterialManager::getSingleton().
      create(std::string("BaseLandMat"),
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    Ogre::TextureUnitState* us = mMat->getTechnique(0)->getPass(0)->createTextureUnitState("_land_default.dds");
    us->setTextureScale(0.1f/vd,0.1f/vd);

    mMat->getTechnique(0)->getPass(0)->setDepthBias(-1);
  }

  void destroyMaterial()
  {
    mMat->getCreator()->remove(mMat->getHandle());
    mMat = Ogre::MaterialPtr();
  }

  ///the created mesh
  Ogre::ManualObject* mObject;

  ///The material for the mesh
  Ogre::MaterialPtr mMat;

  ///scene node for the mesh
  Ogre::SceneNode* mNode;

  ///In essence, the farViewDistance of the camera last frame
  Ogre::Real mMeshDistance;

  Ogre::SceneNode* mTerrainSceneNode;
};
