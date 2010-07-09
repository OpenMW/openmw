#ifndef OENGINE_OGRE_RENDERER_H
#define OENGINE_OGRE_RENDERER_H

/*
  Ogre renderer class
 */

#include <OgreRoot.h>
#include <string>

namespace Ogre
{
    class Root;
    class RenderWindow;
    class SceneManager;
    class Camera;
    class Viewport;
}

namespace Render
{
  class OgreRenderer
  {
    Ogre::Root *mRoot;
    Ogre::RenderWindow *mWindow;
    Ogre::SceneManager *mScene;
    Ogre::Camera *mCamera;
    Ogre::Viewport *mView;
    bool logging;

  public:
    OgreRenderer()
      : mRoot(NULL), mWindow(NULL), mScene(NULL) {}
    ~OgreRenderer() { cleanup(); }

    /** Configure the renderer. This will load configuration files and
        set up the Root and logging classes. */
    bool configure(bool showConfig,     // Show config dialog box?
                   const std::string &pluginCfg, // plugin.cfg file
                   bool _logging);      // Enable or disable logging

    /// Create a window with the given title
    void createWindow(const std::string &title);

    /// Set up the scene manager, camera and viewport
    void createScene(const std::string camName="Camera",// Camera name
                     float fov=55,                      // Field of view angle
                     float nearClip=5                   // Near clip distance
                     );

    /// Kill the renderer.
    void cleanup();

    /// Start the main rendering loop
    void start() { mRoot->startRendering(); }

    /// Write a screenshot to file
    void screenshot(const std::string &file);

    /// Get the Root
    Ogre::Root *getRoot() { return mRoot; }

    /// Get the rendering window
    Ogre::RenderWindow *getWindow() { return mWindow; }

    /// Get the scene manager
    Ogre::SceneManager *getScene() { return mScene; }

    /// Camera
    Ogre::Camera *getCamera() { return mCamera; }

    /// Viewport
    Ogre::Viewport *getViewport() { return mView; }
  };
}

#endif
