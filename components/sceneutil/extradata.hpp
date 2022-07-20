#ifndef OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H
#define OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H

#include <array>

#include <osg/NodeVisitor>
#include <osg/StateAttribute>

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
    class ProcessExtraDataVisitor : public osg::NodeVisitor
    {
    public:
        ProcessExtraDataVisitor(Resource::SceneManager* sceneMgr) : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), mSceneMgr(sceneMgr) {}

        void apply(osg::Node& node) override;

    private:
        void setupSoftEffect(osg::Node& node, float size, bool falloff);

        Resource::SceneManager* mSceneMgr;
    };
}

#endif
