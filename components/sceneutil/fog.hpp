#ifndef OPENMW_COMPONENTS_SCENEUTIL_FOG_H
#define OPENMW_COMPONENTS_SCENEUTIL_FOG_H

namespace osg
{
    class StateSet;
    class Vec4f;
}

namespace SceneUtil
{

    class FogUniformController
    {
    public:
        FogUniformController(osg::StateSet& stateset);

        void setEnd(float end) const;

        void setStart(float start) const;

        void setColor(const osg::Vec4f& color);

        void disable();

    private:
        osg::StateSet& mStateset;
    };

}

#endif