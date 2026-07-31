#include "fog.hpp"

#include <osg/StateSet>

namespace SceneUtil
{

    FogUniformController::FogUniformController(osg::StateSet& stateset)
        : mStateset(stateset)
    {
    }

    void FogUniformController::setEnd(float end) const
    {
        mStateset.getOrCreateUniform("fog.end", osg::Uniform::FLOAT)->set(end);
    }

    void FogUniformController::setStart(float start) const
    {
        mStateset.getOrCreateUniform("fog.start", osg::Uniform::FLOAT)->set(start);
    }

    void FogUniformController::setColor(const osg::Vec4f& color)
    {
        mStateset.getOrCreateUniform("fog.color", osg::Uniform::FLOAT_VEC4)->set(color);
    }

    void FogUniformController::disable()
    {
        constexpr float effectivelyDisabledFogDistance = 10000000.f;
        setStart(effectivelyDisabledFogDistance);
        setEnd(effectivelyDisabledFogDistance);
    }

}