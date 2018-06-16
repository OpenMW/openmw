#ifndef OPENMW_COMPONENTS_SDLUTIL_IMAGETOSURFACE_H
#define OPENMW_COMPONENTS_SDLUTIL_IMAGETOSURFACE_H

#include <memory>

struct SDL_Surface;

namespace osg
{
    class Image;
}

namespace SDLUtil
{
    typedef std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)> SurfaceUniquePtr;

    /// Convert an osg::Image to an SDL_Surface.
    SurfaceUniquePtr imageToSurface(osg::Image* image, bool flip=false);

}

#endif
