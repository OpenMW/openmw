#ifndef _GAME_RENDER_MWSCENE_H
#define _GAME_RENDER_MWSCENE_H

#include "ogre/renderer.hpp"

namespace Ogre
{
    class Camera;
    class Viewport;
    class SceneManager;
    class SceneNode;
}

namespace Render
{
  /** Class responsible for Morrowind-specific interfaces to OGRE.
   */
  class MWScene
  {
    OgreRenderer *rend;
    Ogre::SceneManager *sceneMgr;
    Ogre::Camera *camera;
    Ogre::Viewport *vp;

    // Root node for all objects added to the scene. This is rotated so
    // that the OGRE coordinate system matches that used internally in
    // Morrowind.
    Ogre::SceneNode *mwRoot;

  public:
    void setup(OgreRenderer *_rend);
  };
}

#endif
