#include "imagetosurface.hpp"

#include <osg/Image>
#include <SDL_surface.h>

namespace SDLUtil
{

SurfaceUniquePtr imageToSurface(osg::Image *image, bool flip)
{
    int width = image->s();
    int height = image->t();
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);

    for(int x = 0; x < width; ++x)
        for(int y = 0; y < height; ++y)
        {
            osg::Vec4f clr = image->getColor(x, flip ? ((height-1)-y) : y);
            int bpp = surface->format->BytesPerPixel;
            Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
            *(Uint32*)(p) = SDL_MapRGBA(surface->format, static_cast<Uint8>(clr.r() * 255),
                static_cast<Uint8>(clr.g() * 255), static_cast<Uint8>(clr.b() * 255), static_cast<Uint8>(clr.a() * 255));
        }

    return SurfaceUniquePtr(surface, SDL_FreeSurface);
}

}
