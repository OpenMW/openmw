#ifndef _OGRE_RENDERER_H
#define _OGRE_RENDERER_H

/*
  Ogre renderer class
 */

#include <OgreRoot.h>
#include <string>

namespace Ogre
{
    class Root;
    class RenderWindow;
}

namespace Render
{
  class OgreRenderer
  {
    Ogre::Root *mRoot;
    Ogre::RenderWindow *mWindow;
    bool logging;

  public:
    OgreRenderer()
      : mRoot(NULL) {}
    ~OgreRenderer() { cleanup(); }

    /** Configure the renderer. This will load configuration files and
        set up the Root and logging classes. */
    bool configure(bool showConfig,     // Show config dialog box?
                   const std::string &pluginCfg, // plugin.cfg file
                   bool _logging);      // Enable or disable logging

    /// Create a window with the given title
    void createWindow(const std::string &title);

    /// Kill the renderer.
    void cleanup();

    /// Start the main rendering loop
    void start() { mRoot->startRendering(); }

    /// Get the Root
    Ogre::Root *getRoot() { return mRoot; }

    /// Get the rendering window
    Ogre::RenderWindow *getWindow() { return mWindow; }
  };
}

#endif
