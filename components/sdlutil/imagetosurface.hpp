#ifndef OPENMW_COMPONENTS_SDLUTIL_IMAGETOSURFACE_H
#define OPENMW_COMPONENTS_SDLUTIL_IMAGETOSURFACE_H

struct SDL_Surface;

namespace osg
{
    class Image;
}

namespace SDLUtil
{

    /// Convert an osg::Image to an SDL_Surface.
    /// @note The returned surface must be freed using SDL_FreeSurface.
    SDL_Surface* imageToSurface(osg::Image* image, bool flip=false);

}

#endif
