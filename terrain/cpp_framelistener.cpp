class TerrainFrameListener : public FrameListener
{
protected:
  Terrain* mTerrain;
  MWHeightmap* mHeights;

  /**
   * Updates the quad tree
   */
  bool frameEnded(const FrameEvent& evt)
  {
    mTerrain->update(evt.timeSinceLastFrame);
    return true;
  }

public:
  void setup()
  {
    // Add the frame listener
    mRoot->addFrameListener(this);

    //our derived heightmap
    mHeights = new MWHeightmap();
    mHeights->load(TERRAIN_OUTPUT);

    //setup terrain
    mTerrain = new Terrain( mHeights, //heightmap
                            mSceneMgr->getRootSceneNode()->createChildSceneNode("TERRAIN_ROOT"));  //root scene node

    //fix settings
    mTerrain->setMorphingEnabled(false);
    mTerrain->setTextureFadingEnabled(false);

    //create the quad node
    mTerrain->create();
  }

  /* KILLME
  void drawLine(std::string name, Ogre::Vector3 start, Ogre::Vector3 end)
  {
    Ogre::ManualObject* mo =  mSceneMgr->createManualObject( name);
    Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode( name+"node");

    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create( name+"Material","debugger");
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0,0,1,0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0,0,1);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,1);

    mo->begin(name+"Material", Ogre::RenderOperation::OT_LINE_LIST);
    mo->position(start);
    mo->position(end);
    mo->end();
    node->attachObject(mo);
  }
  */
};
