#include "fog.hpp"

#include <osg/StateSet>

namespace SceneUtil
{

    void setFogEnd(osg::StateSet& stateset, float end, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.end");
        stateset.addUniform(new osg::Uniform("fog.end", end), value);
    }

    void setFogStart(osg::StateSet& stateset, float start, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.start");
        stateset.addUniform(new osg::Uniform("fog.start", start), value);
    }

    void setFogColor(osg::StateSet& stateset, const osg::Vec4f& color, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.color");
        stateset.addUniform(new osg::Uniform("fog.color", color), value);
    }

    void disableFog(osg::StateSet& stateset, osg::StateAttribute::OverrideValue value)
    {
        constexpr float effectivelyDisabledFogDistance = 10000000.f;
        setFogStart(stateset, effectivelyDisabledFogDistance, value);
        setFogEnd(stateset, effectivelyDisabledFogDistance, value);
    }

}