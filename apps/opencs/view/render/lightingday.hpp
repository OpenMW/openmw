#ifndef OPENCS_VIEW_LIGHTING_DAY_H
#define OPENCS_VIEW_LIGHTING_DAY_H

#include "lighting.hpp"

namespace Ogre
{
    class Light;
}

namespace CSVRender
{
    class LightingDay : public Lighting
    {
            osgViewer::View* mView;

        public:

            LightingDay();

            virtual void activate (osgViewer::View* view,
                const osg::Vec4f *defaultAmbient = 0);

            virtual void deactivate();

            virtual void setDefaultAmbient (const osg::Vec4f& colour);
    };
}

#endif
