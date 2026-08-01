#ifndef OPENMW_COMPONENTS_SCENEUTIL_FOG_H
#define OPENMW_COMPONENTS_SCENEUTIL_FOG_H

namespace osg
{
    class StateSet;
    class Vec4f;
}

namespace SceneUtil
{

    void setFogEnd(osg::StateSet& stateset, float end);

    void setFogStart(osg::StateSet& stateset, float start);

    void setFogColor(osg::StateSet& stateset, const osg::Vec4f& color);

    void disableFog(osg::StateSet& stateset);

}

#endif