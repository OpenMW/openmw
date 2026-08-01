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

    void disableFog(osg::StateSet& stateset, osg::StateAttribute::OverrideValue value = osg::StateAttribute::ON);

}

#endif