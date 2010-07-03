#ifndef MANGLE_REND2D_DRIVER_H
#define MANGLE_REND2D_DRIVER_H

#include <string>
#include "sprite.hpp"

namespace Mangle
{
  namespace Rend2D
  {
    /**
       The driver is the connection to the backend system that powers
       2D sprite rendering. For example the backend could be SDL or
       any other 2D-capable graphics library.
    */
    struct Driver
    {
      /// Get the screen sprite
      virtual Sprite *getScreen() = 0;

      /// Sets the video mode.
      virtual void setVideoMode(int width, int height, int bpp=32, bool fullscreen=false) = 0;

      /** Update the screen. Until this function is called, none of
          the changes written to the screen sprite will be visible.
      */
      virtual void update() = 0;

      /// Set the window title, as well as the title of the window
      /// when "iconified"
      virtual void setWindowTitle(const std::string &title,
                                  const std::string &icon) = 0;

      /// Set the window title
      void setWindowTitle(const std::string &title) { setWindowTitle(title,title); }

      /// Load sprite from an image file
      virtual Sprite* loadImage(const std::string &file) = 0;

      /// Load a sprite from an image file stored in memory.
      virtual Sprite* loadImage(const void* data, size_t size) = 0;

      /** @brief Set gamma value for all colors.

          Note: Setting this in windowed mode will affect the ENTIRE
          SCREEN!
      */
      virtual void setGamma(float gamma) = 0;

      /// Set gamma individually for red, green, blue
      virtual void setGamma(float red, float green, float blue) = 0;

      /// Get screen width
      virtual int width() = 0;

      /// Get screen height
      virtual int height() = 0;
    };
  }
}
#endif
