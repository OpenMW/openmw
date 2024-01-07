#ifndef OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H
#define OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H

#include <osg/NodeVisitor>

namespace Resource
{
    class SceneManager;
}

namespace osg
{
    class Node;
}

namespace SceneUtil
{
    void setupSoftEffect(osg::Node& node, float size, bool falloff, float falloffDepth);
    void setupDistortion(osg::Node& node, float distortionStrength);

    class ProcessExtraDataVisitor : public osg::NodeVisitor
    {
    public:
        ProcessExtraDataVisitor(Resource::SceneManager* sceneMgr)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mSceneMgr(sceneMgr)
        {
        }

        void apply(osg::Node& node) override;

    private:
        Resource::SceneManager* mSceneMgr;
    };
}

#endif
