#ifndef OPENCS_VIEW_LIGHTING_DAY_H
#define OPENCS_VIEW_LIGHTING_DAY_H

#include "lighting.hpp"

namespace CSVRender
{
    class LightingDay : public Lighting
    {
        public:

            LightingDay();

            virtual void activate (osg::Group* rootNode, bool /*isExterior*/);

            virtual void deactivate();

            virtual osg::Vec4f getAmbientColour(osg::Vec4f *defaultAmbient);
    };
}

#endif
