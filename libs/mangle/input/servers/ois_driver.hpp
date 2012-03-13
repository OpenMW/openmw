#ifndef MANGLE_INPUT_OIS_DRIVER_H
#define MANGLE_INPUT_OIS_DRIVER_H

#include "../driver.hpp"

namespace OIS
{
  class InputManager;
  class Mouse;
  class Keyboard;
}

namespace Ogre
{
  class RenderWindow;
}

namespace Mangle
{
  namespace Input
  {
    struct OISListener;

    /** Input driver for OIS, the input manager typically used with
        Ogre.
     */
    struct OISDriver : Driver
    {
      /// If exclusive=true, then we capture mouse and keyboard from
      /// the OS.
      OISDriver(Ogre::RenderWindow *window, bool exclusive=true);
      ~OISDriver();

      void capture();
      bool isDown(int index);
      /// Not currently supported.
      void showMouse(bool) {}

    private:
      OIS::InputManager *inputMgr;
      OIS::Mouse *mouse;
      OIS::Keyboard *keyboard;

      OISListener *listener;
    };
  }
}
#endif
