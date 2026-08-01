#include "fog.hpp"

#include <osg/StateSet>

namespace SceneUtil
{

    void setFogEnd(osg::StateSet& stateset, float end)
    {
        stateset.getOrCreateUniform("fog.end", osg::Uniform::FLOAT)->set(end);
    }

    void setFogStart(osg::StateSet& stateset, float start)
    {
        stateset.getOrCreateUniform("fog.start", osg::Uniform::FLOAT)->set(start);
    }

    void setFogColor(osg::StateSet& stateset, const osg::Vec4f& color)
    {
        stateset.getOrCreateUniform("fog.color", osg::Uniform::FLOAT_VEC4)->set(color);
    }

    void disableFog(osg::StateSet& stateset)
    {
        constexpr float effectivelyDisabledFogDistance = 10000000.f;
        setFogStart(stateset, effectivelyDisabledFogDistance);
        setFogEnd(stateset, effectivelyDisabledFogDistance);
    }

}