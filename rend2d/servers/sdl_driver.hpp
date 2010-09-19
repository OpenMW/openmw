#ifndef MANGLE_DRAW2D_SDL_H
#define MANGLE_DRAW2D_SDL_H

#include "../driver.hpp"

// Predeclarations keep the streets safe at night
struct SDL_Surface;
struct SDL_RWops;

namespace Mangle
{
  namespace Rend2D
  {
    /// SDL-implementation of Sprite
    struct SDL_Sprite : Sprite
    {
      /** Draw a sprite in the given position. Can only draw other SDL
          sprites.
      */
      void draw(Sprite *s,                // Must be SDL_Sprite
                int x, int y,             // Destination position
                int sx=0, int sy=0,       // Source position
                int w=-1, int h=-1        // Amount to draw. -1 means remainder.
                );

      SDL_Sprite(SDL_Surface *s, bool autoDelete=true);
      ~SDL_Sprite();

      // Information retrieval
      int width();
      int height();
      SDL_Surface *getSurface() { return surface; }

      // Fill with a given pixel value
      void fill(int value);

      // Set one pixel
      void pixel(int x, int y, int value);

      const SpriteData *lock();
      void unlock();

    private:
      // The SDL surface
      SDL_Surface* surface;

      // Used for locking
      SpriteData data;

      // If true, delete this surface when the canvas is destructed
      bool autoDel;
    };

    class SDLDriver : public Driver
    {
      // The main display surface
      SDL_Sprite *display;

      // The actual display surface. May or may not be the same
      // surface pointed to by 'display' above, depending on the
      // softDouble flag.
      SDL_Surface *realDisp;

      // If true, we do software double buffering.
      bool softDouble;

    public:
      SDLDriver();
      ~SDLDriver();

      /// Sets the video mode. Will create the window if it is not
      /// already set up. Note that for SDL, bpp=0 means use current
      /// bpp.
      void setVideoMode(int width, int height, int bpp=0, bool fullscreen=false);

      /// Update the screen
      void update();

      /// Set the window title, as well as the title of the window
      /// when "iconified"
      void setWindowTitle(const std::string &title,
                          const std::string &icon);

      // Include overloads from our Glorious parent
      using Driver::setWindowTitle;

      /// Load sprite from an image file, using SDL_image.
      Sprite* loadImage(const std::string &file);

      /// Load sprite from an SDL_RWops structure. autoFree determines
      /// whether the RWops struct should be closed/freed after use.
      Sprite* loadImage(SDL_RWops *src, bool autoFree=false);

      /// Load a sprite from an image file stored in memory. Uses
      /// SDL_image.
      Sprite* loadImage(const void* data, size_t size);

      /// Set gamma value
      void setGamma(float gamma) { setGamma(gamma,gamma,gamma); }

      /// Set gamma individually for red, green, blue
      void setGamma(float red, float green, float blue);

      /// Convert an existing SDL surface into a sprite
      Sprite* spriteFromSDL(SDL_Surface *surf, bool autoFree = true);

      // Get width and height
      int width()  { return display ? display->width()  : 0; }
      int height() { return display ? display->height() : 0; }

      /// Get the screen sprite
      Sprite *getScreen() { return display; }

      /// Not really a graphic-related function, but very
      /// handly. Sleeps the given number of milliseconds using
      /// SDL_Delay().
      void sleep(int ms);

      /// Get the number of ticks since SDL initialization, using
      /// SDL_GetTicks().
      unsigned int ticks();
    };
  }
}
#endif
