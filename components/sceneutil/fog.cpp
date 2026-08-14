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

    void setUnderWaterFogEnd(osg::StateSet& stateset, float end, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underWaterEnd");
        stateset.addUniform(new osg::Uniform("fog.underWaterEnd", end), value);
    }

    void setUnderWaterFogStart(osg::StateSet& stateset, float start, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underWaterStart");
        stateset.addUniform(new osg::Uniform("fog.underWaterStart", start), value);
    }

    void setUnderWaterFogColor(
        osg::StateSet& stateset, const osg::Vec4f& color, osg::StateAttribute::OverrideValue value)
    {
        stateset.removeUniform("fog.underWaterColor");
        stateset.addUniform(new osg::Uniform("fog.underWaterColor", color), value);
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

    void updateUnderWaterFogEnd(osg::StateSet& stateset, float end)
    {
        stateset.getUniform("fog.underWaterEnd")->set(end);
    }

    void updateUnderWaterFogStart(osg::StateSet& stateset, float start)
    {
        stateset.getUniform("fog.underWaterStart")->set(start);
    }

    void updateUnderWaterFogColor(osg::StateSet& stateset, const osg::Vec4f& color)
    {
        stateset.getUniform("fog.underWaterColor")->set(color);
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
        setFogDepth(stateset, -1.f, value);
    }

}