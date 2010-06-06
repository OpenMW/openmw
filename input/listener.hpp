#ifndef _INPUT_LISTENER_H
#define _INPUT_LISTENER_H

#include "oismanager.hpp"
#include "ogre/renderer.hpp"

#include <OgreFrameListener.h>
#include <OgreRenderWindow.h>

//#include <iostream>

namespace Input
{
  struct ExitListener : Ogre::FrameListener,
                        OIS::KeyListener,
                        OIS::MouseListener
  {
    ExitListener(Render::OgreRenderer &rend,
                 Input::OISManager &input)
      : doExit(false)
    {
      // Set up component pointers
      mWindow = rend.getWindow();
      mMouse = input.mouse;
      mKeyboard = input.keyboard;

      // Add ourself to the managers
      rend.getRoot() -> addFrameListener(this);
      mKeyboard      -> setEventCallback(this);
      mMouse         -> setEventCallback(this);
    }

    // Call this to exit the main loop
    void exitNow() { doExit = true; }

    bool frameStarted(const Ogre::FrameEvent &evt)
    {
      if(mWindow->isClosed() || doExit)
        return false;

      // Capture keyboard and mouse events
      mKeyboard->capture();
      mMouse->capture();

      return Ogre::FrameListener::frameStarted(evt);
    }

    bool keyPressed( const OIS::KeyEvent &arg )
    {
      /*
      std::cout << "KeyPressed {" << arg.key
                << ", " << ((OIS::Keyboard*)(arg.device))->getAsString(arg.key)
                << "} || Character (" << (char)arg.text << ")\n";
      */
      using namespace OIS;

      if(arg.key == KC_Q ||
         arg.key == KC_ESCAPE)
        exitNow();

      return true;
    }

    bool keyReleased( const OIS::KeyEvent &arg )
    {
      return true;
    }

    bool mouseMoved( const OIS::MouseEvent &arg )
    {
      return true;
    }

    bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
      return true;
    }

    bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
      return true;
    }

    private:

    Ogre::RenderWindow *mWindow;
    OIS::Mouse *mMouse;
    OIS::Keyboard *mKeyboard;
    bool doExit;
  };
}
#endif
