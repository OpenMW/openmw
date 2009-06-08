class BaseLand
{
public:
  BaseLand()
  {
    createMesh();
  }

  ~BaseLand()
  {
    destroyMesh();
  }

  // Repositions the mesh based on camera location
  void update()
  {
    Ogre::Real vd = mCamera->getFarClipDistance();
    // Recreate the mesh if the view distance has increased
    if ( vd > mMeshDistance  )
      {
        destroyMesh();
        createMesh();
      }

    Ogre::Vector3 p = mCamera->getDerivedPosition();
    p.x -= ((int)p.x % CELL_WIDTH);
    p.z -= ((int)p.z % CELL_WIDTH);

    float h = (p.y + 2048)*2.0/CELL_WIDTH;
    h *= h;

    mNode->setPosition(p.x, -p.z, -32 - h);
  }

private:
  void createMesh()
  {
    float vd = mCamera->getFarClipDistance();

    mMeshDistance = vd;

    vd = vd/CELL_WIDTH * 32;

    mMat = Ogre::MaterialManager::getSingleton().
      create("BaseLandMat",
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    Ogre::TextureUnitState* us = mMat->getTechnique(0)->getPass(0)->createTextureUnitState("_land_default.dds");
    us->setTextureScale(1.0f/vd,1.0f/vd);

    mMat->getTechnique(0)->getPass(0)->setDepthBias(-1);

    mObject = mSceneMgr->createManualObject("BaseLand");
    mObject->begin("BaseLandMat", Ogre::RenderOperation::OT_TRIANGLE_LIST);

    vd = mMeshDistance;

    mObject->position(-vd,vd,-2048);
    mObject->textureCoord(0, 1);

    mObject->position(-vd,-vd,-2048);
    mObject->textureCoord(0, 0);

    mObject->position(vd,-vd,-2048);
    mObject->textureCoord(1, 0);

    mObject->position(vd,vd,-2048);
    mObject->textureCoord(1, 1);

    mObject->quad(0,1,2,3);

    mObject->end();

    mNode = g_rootTerrainNode->createChildSceneNode();
    mNode->attachObject(mObject);
  }

  void destroyMesh()
  {
    mNode->detachAllObjects();
    mSceneMgr->destroyManualObject(mObject);
    mNode->getParentSceneNode()->removeAndDestroyChild(mNode->getName());

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
};
