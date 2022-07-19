#ifndef COMPONENTS_TERRAIN_VIEW_H
#define COMPONENTS_TERRAIN_VIEW_H

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Vec3f>

namespace Terrain
{
    /**
     * @brief A View is a collection of rendering objects that are visible from a given camera/intersection.
     * The base View class is part of the interface for usage in conjunction with preload feature.
     */
    class View : public osg::Referenced
    {
    public:
        virtual ~View() {}

        /// Reset internal structure so that the next addition to the view will override the previous frame's contents.
        virtual void reset() = 0;
    };

}

#endif
