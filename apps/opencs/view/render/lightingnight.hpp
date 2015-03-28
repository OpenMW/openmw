#ifndef OPENCS_VIEW_LIGHTING_NIGHT_H
#define OPENCS_VIEW_LIGHTING_NIGHT_H

#include "lighting.hpp"

namespace Ogre
{
    class Light;
}

namespace CSVRender
{
    class LightingNight : public Lighting
    {
            osgViewer::View* mView;

        public:

            LightingNight();

            virtual void activate (osgViewer::View* view,
                const osg::Vec4f *defaultAmbient = 0);

            virtual void deactivate();

            virtual void setDefaultAmbient (const osg::Vec4f& colour);
    };
}

#endif
