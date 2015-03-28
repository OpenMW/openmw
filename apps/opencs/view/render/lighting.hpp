#ifndef OPENCS_VIEW_LIGHTING_H
#define OPENCS_VIEW_LIGHTING_H

namespace osgViewer
{
    class View;
}
namespace osg
{
    class Vec4f;
}

namespace CSVRender
{
    class Lighting
    {
        public:

            virtual ~Lighting();

            virtual void activate (osgViewer::View* view,
                const osg::Vec4f *defaultAmbient = 0) = 0;

            virtual void deactivate() = 0;

            virtual void setDefaultAmbient (const osg::Vec4f& colour) = 0;
    };
}

#endif
