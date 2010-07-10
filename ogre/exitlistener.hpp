#ifndef OENGINE_OGRE_EXITLISTEN_H
#define OENGINE_OGRE_EXITLISTEN_H

/*
  This FrameListener simply exits the rendering loop when the window
  is closed. You can also tell it to exit manually by setting the exit
  member to true;
 */

#include <OgreFrameListener.h>
#include <OgreRenderWindow.h>

namespace Render
{
  struct ExitListener : Ogre::FrameListener
  {
    Ogre::RenderWindow *window;
    bool exit;

    ExitListener(Ogre::RenderWindow *wnd)
      : window(wnd), exit(false) {}

    bool frameStarted(const FrameEvent &evt)
    {
      if(window->isClosed())
        exit = true;

      return !exit;
    }
  };
}

#endif
