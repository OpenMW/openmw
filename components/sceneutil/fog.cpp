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

    void setFogDepth(osg::StateSet& stateset, float depth, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.depth");
        stateset.addUniform(new osg::Uniform("fog.depth", depth), value);
    }

    void setUnderwaterFogEnd(osg::StateSet& stateset, float end, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underwaterEnd");
        stateset.addUniform(new osg::Uniform("fog.underwaterEnd", end), value);
    }

    void setUnderwaterFogStart(osg::StateSet& stateset, float start, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underwaterStart");
        stateset.addUniform(new osg::Uniform("fog.underwaterStart", start), value);
    }

    void setUnderwaterFogColor(
        osg::StateSet& stateset, const osg::Vec4f& color, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underwaterColor");
        stateset.addUniform(new osg::Uniform("fog.underwaterColor", color), value);
    }

    void updateFogEnd(osg::StateSet& stateset, float end)
    {
        stateset.getUniform("fog.end")->set(end);
    }

    void updateFogStart(osg::StateSet& stateset, float start)
    {
        stateset.getUniform("fog.start")->set(start);
    }

    void updateFogColor(osg::StateSet& stateset, const osg::Vec4f& color)
    {
        stateset.getUniform("fog.color")->set(color);
    }

    void updateUnderwaterFogEnd(osg::StateSet& stateset, float end)
    {
        stateset.getUniform("fog.underwaterEnd")->set(end);
    }

    void updateUnderwaterFogStart(osg::StateSet& stateset, float start)
    {
        stateset.getUniform("fog.underwaterStart")->set(start);
    }

    void updateUnderwaterFogColor(osg::StateSet& stateset, const osg::Vec4f& color)
    {
        stateset.getUniform("fog.underwaterColor")->set(color);
    }

    void updateFogDepth(osg::StateSet& stateset, float depth)
    {
        stateset.getUniform("fog.depth")->set(depth);
    }

    void disableFog(osg::StateSet& stateset, osg::StateAttribute::OverrideValue value)
    {
        constexpr float effectivelyDisabledFogDistance = 10000000.f;
        setFogStart(stateset, effectivelyDisabledFogDistance, value);
        setFogEnd(stateset, effectivelyDisabledFogDistance, value);
        setUnderwaterFogStart(stateset, effectivelyDisabledFogDistance, value);
        setUnderwaterFogEnd(stateset, effectivelyDisabledFogDistance, value);
        setFogDepth(stateset, -1.f, value);
    }

}