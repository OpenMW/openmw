#ifndef OPENCS_VIEW_LIGHTING_NIGHT_H
#define OPENCS_VIEW_LIGHTING_NIGHT_H

#include "lighting.hpp"

namespace CSVRender
{
    class LightingNight : public Lighting
    {
        public:

            LightingNight();

            virtual void activate (osg::Group* rootNode, bool isExterior);
            virtual void deactivate();

            virtual osg::Vec4f getAmbientColour(osg::Vec4f *defaultAmbient);
    };
}

#endif
