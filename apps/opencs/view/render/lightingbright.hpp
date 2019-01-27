#ifndef OPENCS_VIEW_LIGHTING_BRIGHT_H
#define OPENCS_VIEW_LIGHTING_BRIGHT_H

#include "lighting.hpp"

namespace osg
{
    class Light;
    class Group;
}

namespace CSVRender
{
    class LightingBright : public Lighting
    {
        public:

            LightingBright();

            virtual void activate (osg::Group* rootNode, bool /*isExterior*/);

            virtual void deactivate();

            virtual osg::Vec4f getAmbientColour(osg::Vec4f* defaultAmbient);
    };
}

#endif
