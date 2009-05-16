class TerrainFrameListener : public FrameListener
{
protected:
  /**
   * Updates the quad tree
   */
  bool frameEnded(const FrameEvent& evt)
  {
    g_heightMap->update(evt.timeSinceLastFrame);
    return true;
  }

public:
  void setup()
  {
    // Add the frame listener
    mRoot->addFrameListener(this);

    // Create a root scene node first
    Ogre::SceneNode *node = mSceneMgr->getRootSceneNode()
      ->createChildSceneNode("TERRAIN_ROOT");

    // The main terrain object
    g_heightMap = new HeightMap(node);
    g_heightMap->load(TERRAIN_OUTPUT);

    //fix settings
    g_heightMap->setMorphingEnabled(false);
    g_heightMap->setTextureFadingEnabled(false);

    //create the quad node
    g_heightMap->create();
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
