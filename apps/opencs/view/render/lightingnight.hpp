#ifndef OPENCS_VIEW_LIGHTING_NIGHT_H
#define OPENCS_VIEW_LIGHTING_NIGHT_H

#include "lighting.hpp"

namespace CSVRender
{
    class LightingNight : public Lighting
    {
        public:

            LightingNight();

            void activate (osg::Group* rootNode, bool isExterior) override;
            void deactivate() override;

            osg::Vec4f getAmbientColour(osg::Vec4f *defaultAmbient) override;
    };
}

#endif
