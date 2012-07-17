#include "sdl_gl_driver.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <stdexcept>
#include <cassert>

using namespace Mangle::Rend2D;

void SDLGL_Sprite::draw(Sprite *s,              // Must be SDLGL_Sprite
                      int x, int y,             // Destination position
                      int sx, int sy,           // Source position
                      int w, int h              // Amount to draw. -1 means remainder.
                      )
{
  // Get source surface
  SDLGL_Sprite *other = dynamic_cast<SDLGL_Sprite*>(s);
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

SDLGL_Sprite::SDLGL_Sprite(SDL_Surface *s, bool autoDelete)
  : surface(s), autoDel(autoDelete)
{
  assert(surface != NULL);
}

SDLGL_Sprite::~SDLGL_Sprite()
{
  if(autoDel)
    SDL_FreeSurface(surface);
}

void SDLGL_Sprite::fill(int value)
{
  SDL_FillRect(surface, NULL, value);
}

int SDLGL_Sprite::width()  { return surface->w; }
int SDLGL_Sprite::height() { return surface->h; }

SDLGLDriver::SDLGLDriver() : display(NULL), realDisp(NULL)
{
  if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
    throw std::runtime_error("Error initializing SDL video");
}
SDLGLDriver::~SDLGLDriver()
{
  if(display) delete display;
  SDL_Quit();
}

// Surface used for the screen. Since OpenGL surfaces must have sizes
// that are powers of 2, we have to "fake" the returned display size
// to match the screen, not the surface itself. If we don't use this,
// the client program will get confused about the actual size of our
// screen, thinking it is bigger than it is.
struct FakeSizeSprite : SDLGL_Sprite
{
  int fakeW, fakeH;

  FakeSizeSprite(SDL_Surface *s, int fw, int fh)
    : SDLGL_Sprite(s), fakeW(fw), fakeH(fh)
  {}

  int width() { return fakeW; }
  int height() { return fakeH; }
};

static int makePow2(int num)
{
  assert(num);
  if((num & (num-1)) != 0)
    {
      int cnt = 0;
      while(num)
        {
          num >>= 1;
          cnt++;
        }
      num = 1 << cnt;
    }
  return num;
}

void SDLGLDriver::setVideoMode(int width, int height, int bpp, bool fullscreen)
{
  unsigned int flags;

  if(display) delete display;

  flags = SDL_OPENGL;

  if (fullscreen)
    flags |= SDL_FULLSCREEN;

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );

  // Create the surface and check it
  screen = SDL_SetVideoMode(width, height, bpp, flags);
  if(screen == NULL)
    throw std::runtime_error("Failed setting SDL video mode");

  // Expand width and height to be powers of 2
  int width2 = makePow2(width);
  int height2 = makePow2(height);

  // Create a new SDL surface of this size
  const SDL_PixelFormat& fmt = *(screen->format);
  realDisp = SDL_CreateRGBSurface(SDL_SWSURFACE,width2,height2,
                                  fmt.BitsPerPixel,
                                  fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask);

  // Create a sprite directly representing the display surface. This
  // allows the user to blit to it directly.
  display = new FakeSizeSprite(realDisp, width, height);

  // Set up the OpenGL format
  nOfColors = fmt.BytesPerPixel;

  if(nOfColors == 4)
    {
      if (fmt.Rmask == 0x000000ff)
        texture_format = GL_RGBA;
      else
        texture_format = GL_BGRA;
    }
  else if(nOfColors == 3)
    {
      if (fmt.Rmask == 0x000000ff)
        texture_format = GL_RGB;
      else
        texture_format = GL_BGR;
    }
  else
    assert(0 && "unsupported screen format");

  glEnable(GL_TEXTURE_2D);

  // Have OpenGL generate a texture object handle for us
  glGenTextures( 1, &texture );

  // Bind the texture object
  glBindTexture( GL_TEXTURE_2D, texture );

  // Set the texture's stretching properties
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
}

void SDLGLDriver::updateNoSwap()
{
  if(!realDisp) return;

  // Fist, set up the screen texture:

  // Bind the texture object
  glBindTexture( GL_TEXTURE_2D, texture );

  // Edit the texture object's image data
  glTexImage2D( GL_TEXTURE_2D, 0, nOfColors, realDisp->w, realDisp->h, 0,
                texture_format, GL_UNSIGNED_BYTE, realDisp->pixels );

  glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  // OpenGL barf. Set up the projection to match our screen
  int vPort[4];
  glGetIntegerv(GL_VIEWPORT, vPort);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, vPort[2], 0, vPort[3], -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glBegin( GL_QUADS );

  // Needed to move the screen into the right place
  int diff = screen->h - realDisp->h;

  // Bottom-left vertex (corner)
  glTexCoord2i( 0, 1 );
  glVertex3f(0,diff,0);

  // Bottom-right vertex (corner)
  glTexCoord2i( 1, 1 );
  glVertex3f( realDisp->w, diff, 0.f );

  // Top-right vertex (corner)
  glTexCoord2i( 1, 0 );
  glVertex3f( realDisp->w, screen->h, 0.f );

  // Top-left vertex (corner)
  glTexCoord2i( 0, 0 );
  glVertex3f( 0, screen->h, 0.f );
  glEnd();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void SDLGLDriver::swap()
{
  SDL_GL_SwapBuffers();
}

void SDLGLDriver::update()
{
  updateNoSwap();
  swap();
}

/// Set the window title, as well as the title of the window when
/// "iconified"
void SDLGLDriver::setWindowTitle(const std::string &title,
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
Sprite* SDLGLDriver::loadImage(const std::string &file)
{
  SDL_Surface *surf = IMG_Load(file.c_str());
  surf = convertImage(surf);
  if(surf == NULL)
    throw std::runtime_error("SDL failed to load image file '" + file + "'");
  return spriteFromSDL(surf);
}

/// Load sprite from an SDL_RWops structure. autoFree determines
/// whether the RWops struct should be closed/freed after use.
Sprite* SDLGLDriver::loadImage(SDL_RWops *src, bool autoFree)
{
  SDL_Surface *surf = IMG_Load_RW(src, autoFree);
  surf = convertImage(surf);
  if(surf == NULL)
    throw std::runtime_error("SDL failed to load image");
  return spriteFromSDL(surf);
}

/// Load a sprite from an image file stored in memory. Uses
/// SDL_image.
Sprite* SDLGLDriver::loadImage(const void* data, size_t size)
{
  SDL_RWops *rw = SDL_RWFromConstMem(data, size);
  return loadImage(rw, true);
}

void SDLGLDriver::setGamma(float red, float green, float blue)
{
  SDL_SetGamma(red,green,blue);
}

/// Convert an existing SDL surface into a sprite
Sprite* SDLGLDriver::spriteFromSDL(SDL_Surface *surf, bool autoFree)
{
  assert(surf);
  return new SDLGL_Sprite(surf, autoFree);
}

void SDLGLDriver::sleep(int ms) { SDL_Delay(ms); }
unsigned int SDLGLDriver::ticks() { return SDL_GetTicks(); }
