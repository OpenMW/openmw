#ifndef MANGLE_INPUT_SDL_DRIVER_H
#define MANGLE_INPUT_SDL_DRIVER_H

#include "../driver.hpp"

namespace Mangle
{
  namespace Input
  {
    /** Input driver for SDL. As the input system of SDL is seldomly
        used alone (most often along with the video system), it is
        assumed that you do your own initialization and cleanup of SDL
        before and after using this driver.

        The Event.event() calls will be given the proper EV_ type, the
        key index (for key up/down events), and a pointer to the full
        SDL_Event structure.
     */
    struct SDLDriver : Driver
    {
      void capture();
      bool isDown(int index);
      void showMouse(bool);
    };
  }
}
#endif
