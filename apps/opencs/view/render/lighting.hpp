#ifndef OPENCS_VIEW_LIGHTING_H
#define OPENCS_VIEW_LIGHTING_H

#include <osg/ref_ptr>

namespace osg
{
    class Vec4f;
    class LightSource;
    class Group;
}

namespace CSVRender
{
    class Lighting
    {
        public:

            Lighting() : mRootNode(0) {}
            virtual ~Lighting();

            virtual void activate (osg::Group* rootNode, bool isExterior) = 0;

            virtual void deactivate() = 0;

            virtual osg::Vec4f getAmbientColour(osg::Vec4f* defaultAmbient) = 0;

        protected:

            void updateDayNightMode(int index);

            osg::ref_ptr<osg::LightSource> mLightSource;
            osg::Group* mRootNode;
    };
}

#endif
