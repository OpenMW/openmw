class TerrainFrameListener : public FrameListener
{
protected:
  bool frameEnded(const FrameEvent& evt)
  {
    g_rootQuad->update(evt.timeSinceLastFrame);
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

    // Fix settings
    g_heightMap->setMorphingEnabled(false);
    g_heightMap->setTextureFadingEnabled(false);

    // Create the root quad
    g_rootQuad = new Quad(Quad::QL_ROOT, 0);
  }
};
