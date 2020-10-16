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

            void activate (osg::Group* rootNode, bool /*isExterior*/) override;

            void deactivate() override;

            osg::Vec4f getAmbientColour(osg::Vec4f* defaultAmbient) override;
    };
}

#endif
