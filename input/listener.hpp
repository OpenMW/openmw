#ifndef _INPUT_LISTENER_H
#define _INPUT_LISTENER_H

#include "oismanager.hpp"
#include "ogre/renderer.hpp"
#include "dispatcher.hpp"

#include <OgreFrameListener.h>
#include <OgreRenderWindow.h>

namespace Input
{
  struct InputListener : Ogre::FrameListener,
                         OIS::KeyListener,
                         OIS::MouseListener
  {
    InputListener(Render::OgreRenderer &rend,
                  Input::OISManager &input,
                  const Input::Dispatcher &_disp)
      : doExit(false), disp(_disp)
    {
      // Set up component pointers
      mWindow = rend.getWindow();
      mMouse = input.mouse;
      mKeyboard = input.keyboard;

      assert(mKeyboard);
      assert(mWindow);

      // Add ourself to the managers
      rend.getRoot() -> addFrameListener(this);
      mKeyboard      -> setEventCallback(this);
      mMouse         -> setEventCallback(this);
    }

    void setCamera(Ogre::Camera *cam) { camera = cam; }

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
      // Pass the event to the dispatcher
      disp.event(arg.key, &arg);
      return true;
    }

    bool keyReleased( const OIS::KeyEvent &arg )
    {
      return true;
    }

    bool mouseMoved( const OIS::MouseEvent &arg )
    {
      using namespace Ogre;
      assert(camera);

      // Mouse sensitivity. Should be a config option later.
      const float MS = 0.2;

      float x = arg.state.X.rel * MS;
      float y = arg.state.Y.rel * MS;

      camera->yaw(Degree(-x));

      // The camera before pitching
      Quaternion nopitch = camera->getOrientation();

      camera->pitch(Degree(-y));

      // Apply some failsafe measures against the camera flipping
      // upside down. Is the camera close to pointing straight up or
      // down?
      if(camera->getUp()[1] <= 0.1)
        // If so, undo the last pitch
        camera->setOrientation(nopitch);

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

    const Dispatcher &disp;
    Ogre::RenderWindow *mWindow;
    Ogre::Camera *camera;
    OIS::Mouse *mMouse;
    OIS::Keyboard *mKeyboard;
    bool doExit;
  };
}
#endif
