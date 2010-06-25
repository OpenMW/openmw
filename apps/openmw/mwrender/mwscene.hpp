#ifndef _GAME_RENDER_MWSCENE_H
#define _GAME_RENDER_MWSCENE_H

#include "components/engine/ogre/renderer.hpp"

namespace Ogre
{
    class Camera;
    class Viewport;
    class SceneManager;
    class SceneNode;
}

namespace MWRender
{
  /** Class responsible for Morrowind-specific interfaces to OGRE.

      This might be refactored partially into a non-mw specific
      counterpart in ogre/ at some point.
   */
  class MWScene
  {
    Render::OgreRenderer &rend;
    Ogre::SceneManager *sceneMgr;
    Ogre::Camera *camera;
    Ogre::Viewport *vp;

    // Root node for all objects added to the scene. This is rotated so
    // that the OGRE coordinate system matches that used internally in
    // Morrowind.
    Ogre::SceneNode *mwRoot;

  public:
    MWScene(Render::OgreRenderer &_rend);

    Ogre::SceneNode *getRoot() { return mwRoot; }
    Ogre::SceneManager *getMgr() { return sceneMgr; }
    Ogre::Camera *getCamera() { return camera; }
    Ogre::Viewport *getViewport() { return vp; }
  };
}

#endif
