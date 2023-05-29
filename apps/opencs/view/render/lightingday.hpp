#ifndef OPENCS_VIEW_LIGHTING_DAY_H
#define OPENCS_VIEW_LIGHTING_DAY_H

#include "lighting.hpp"

#include <osg/Vec4f>

namespace osg
{
    class Group;
}

namespace CSVRender
{
    class LightingDay : public Lighting
    {
    public:
        LightingDay() = default;

        void activate(osg::Group* rootNode, bool /*isExterior*/) override;

        void deactivate() override;

        osg::Vec4f getAmbientColour(osg::Vec4f* defaultAmbient) override;
    };
}

#endif
