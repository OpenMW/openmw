#ifndef OPENMW_COMPONENTS_SCENEUTIL_FOG_H
#define OPENMW_COMPONENTS_SCENEUTIL_FOG_H

#include <osg/StateAttribute>

namespace osg
{
    class StateSet;
    class Vec4f;
}

namespace SceneUtil
{

    void setFogEnd(
        osg::StateSet& stateset, float end, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setFogStart(
        osg::StateSet& stateset, float start, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setFogColor(osg::StateSet& stateset, const osg::Vec4f& color,
        osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setUnderwaterFogEnd(
        osg::StateSet& stateset, float end, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setUnderwaterFogStart(
        osg::StateSet& stateset, float start, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setUnderwaterFogColor(osg::StateSet& stateset, const osg::Vec4f& color,
        osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void setFogDepth(
        osg::StateSet& stateset, float depth, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

    void updateFogEnd(osg::StateSet& stateset, float end);

    void updateFogStart(osg::StateSet& stateset, float start);

    void updateFogColor(osg::StateSet& stateset, const osg::Vec4f& color);

    void updateUnderwaterFogEnd(osg::StateSet& stateset, float end);

    void updateUnderwaterFogStart(osg::StateSet& stateset, float start);

    void updateUnderwaterFogColor(osg::StateSet& stateset, const osg::Vec4f& color);

    void updateFogDepth(osg::StateSet& stateset, float depth);

    void disableFog(osg::StateSet& stateset, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

}

#endif