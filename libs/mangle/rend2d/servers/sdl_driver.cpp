#include "sdl_driver.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <stdexcept>
#include <cassert>

using namespace Mangle::Rend2D;

const SpriteData *SDL_Sprite::lock()
{
  // Make sure we aren't already locked
  assert(!data.pixels);

  // Lock the surface and set up the data structure
  SDL_LockSurface(surface);

  data.pixels = surface->pixels;
  data.w = surface->w;
  data.h = surface->h;
  data.pitch = surface->pitch;
  data.bypp = surface->format->BytesPerPixel;

  return &data;
}

void SDL_Sprite::unlock()
{
  if(data.pixels)
    {
      SDL_UnlockSurface(surface);
      data.pixels = NULL;
    }
}

// This is a really crappy and slow implementation, only intended for
// testing purposes. Use lock/unlock for faster pixel drawing.
void SDL_Sprite::pixel(int x, int y, int color)
{
  SDL_LockSurface(surface);

  int bpp = surface->format->BytesPerPixel;
  char *p = (char*)surface->pixels + y*surface->pitch + x*bpp;

  switch(bpp)
    {
    case 1: *p = color; break;
    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
          {
            p[0] = (color >> 16) & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = color & 0xff;
          }
        else
          {
            p[0] = color & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = (color >> 16) & 0xff;
          }
        break;
    case 4:
        *(int*)p = color;
        break;
    }
  SDL_UnlockSurface(surface);
}

void SDL_Sprite::draw(Sprite *s,                // Must be SDL_Sprite
                      int x, int y,             // Destination position
                      int sx, int sy,           // Source position
                      int w, int h              // Amount to draw. -1 means remainder.
                      )
{
  // Get source surface
  SDL_Sprite *other = dynamic_cast<SDL_Sprite*>(s);
  assert(other != NULL);
  SDL_Surface *img = other->getSurface();

  // Check coordinate validity
  assert(sx <= img->w     && sy <= img->h);
  assert(x  <= surface->w && y  <= surface->h);
  assert(sx >= 0 && sy >= 0);

  // Compute width and height if necessary
  if(w == -1) w = img->w - sx;
  if(h == -1) h = img->h - sy;

  // Check them if they're valid
  assert(w >= 0 && w <= img->w);
  assert(h >= 0 && h <= img->h);

  SDL_Rect dest;
  dest.x = x;
  dest.y = y;
  dest.w = w;
  dest.h = h;

  SDL_Rect src;
  src.x = sx;
  src.y = sy;
  src.w = w;
  src.h = h;

  // Do the Blitman
  SDL_BlitSurface(img, &src, surface, &dest);
}

SDL_Sprite::SDL_Sprite(SDL_Surface *s, bool autoDelete)
  : surface(s), autoDel(autoDelete)
{
  assert(surface != NULL);
  data.pixels = NULL;
}

SDL_Sprite::~SDL_Sprite()
{
  if(autoDel)
    SDL_FreeSurface(surface);
}

void SDL_Sprite::fill(int value)
{
  SDL_FillRect(surface, NULL, value);
}

int SDL_Sprite::width()  { return surface->w; }
int SDL_Sprite::height() { return surface->h; }

SDLDriver::SDLDriver() : display(NULL), realDisp(NULL), softDouble(false)
{
  if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
    throw std::runtime_error("Error initializing SDL video");
}
SDLDriver::~SDLDriver()
{
  if(display) delete display;
  SDL_Quit();
}

void SDLDriver::setVideoMode(int width, int height, int bpp, bool fullscreen)
{
  unsigned int flags;

  if(display) delete display;

  if (fullscreen)
    // Assume fullscreen mode allows a double-bufferd hardware
    // mode. We need more test code for this to be safe though.
    flags = SDL_FULLSCREEN | SDL_HWSURFACE | SDL_DOUBLEBUF;
  else
    flags = SDL_SWSURFACE;

  // Create the surface and check it
  realDisp = SDL_SetVideoMode(width, height, bpp, flags);
  if(realDisp == NULL)
    throw std::runtime_error("Failed setting SDL video mode");

  // Code for software double buffering. I haven't found this to be
  // any speed advantage at all in windowed mode (it's slower, as one
  // would expect.) Not properly tested in fullscreen mode with
  // hardware buffers, but it will probably only be an improvement if
  // we do excessive writing (ie. write each pixel on average more
  // than once) or try to read from the display buffer.
  if(softDouble)
    {
      // Make a new surface with the same attributes as the real
      // display surface.
      SDL_Surface *back = SDL_DisplayFormat(realDisp);
      assert(back != NULL);

      // Create a sprite representing the double buffer
      display = new SDL_Sprite(back);
    }
  else
    {
      // Create a sprite directly representing the display surface.
      // The 'false' parameter means do not autodelete the screen
      // surface upon exit (since SDL manages it)
      display = new SDL_Sprite(realDisp, false);
    }
}

/// Update the screen
void SDLDriver::update()
{
  // Blit the soft double buffer onto the real display buffer
  if(softDouble)
    SDL_BlitSurface(display->getSurface(), NULL, realDisp, NULL );

  if(realDisp)
    SDL_Flip(realDisp);
}

/// Set the window title, as well as the title of the window when
/// "iconified"
void SDLDriver::setWindowTitle(const std::string &title,
                               const std::string &icon)
{
  SDL_WM_SetCaption( title.c_str(), icon.c_str() );
}

// Convert the given surface to display format.
static SDL_Surface* convertImage(SDL_Surface* surf)
{
  if(surf != NULL)
    {
      // Convert the image to the display buffer format, for faster
      // blitting
      SDL_Surface *surf2 = SDL_DisplayFormat(surf);
      SDL_FreeSurface(surf);
      surf = surf2;
    }
  return surf;
}

/// Load sprite from an image file, using SDL_image.
Sprite* SDLDriver::loadImage(const std::string &file)
{
  SDL_Surface *surf = IMG_Load(file.c_str());
  surf = convertImage(surf);
  if(surf == NULL)
    throw std::runtime_error("SDL failed to load image file '" + file + "'");
  return spriteFromSDL(surf);
}

/// Load sprite from an SDL_RWops structure. autoFree determines
/// whether the RWops struct should be closed/freed after use.
Sprite* SDLDriver::loadImage(SDL_RWops *src, bool autoFree)
{
  SDL_Surface *surf = IMG_Load_RW(src, autoFree);
  surf = convertImage(surf);
  if(surf == NULL)
    throw std::runtime_error("SDL failed to load image");
  return spriteFromSDL(surf);
}

/// Load a sprite from an image file stored in memory. Uses
/// SDL_image.
Sprite* SDLDriver::loadImage(const void* data, size_t size)
{
  SDL_RWops *rw = SDL_RWFromConstMem(data, size);
  return loadImage(rw, true);
}

void SDLDriver::setGamma(float red, float green, float blue)
{
  SDL_SetGamma(red,green,blue);
}

/// Convert an existing SDL surface into a sprite
Sprite* SDLDriver::spriteFromSDL(SDL_Surface *surf, bool autoFree)
{
  assert(surf);
  return new SDL_Sprite(surf, autoFree);
}

void SDLDriver::sleep(int ms) { SDL_Delay(ms); }
unsigned int SDLDriver::ticks() { return SDL_GetTicks(); }
