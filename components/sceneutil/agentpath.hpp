#ifndef OPENMW_COMPONENTS_SCENEUTIL_AGENTPATH_H
#define OPENMW_COMPONENTS_SCENEUTIL_AGENTPATH_H

#include <osg/ref_ptr>

#include <deque>

namespace osg
{
    class Group;
    class Vec3f;
    class StateSet;
}

namespace DetourNavigator
{
    struct RecastSettings;
    struct AgentBounds;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createAgentPathGroup(const std::deque<osg::Vec3f>& path,
        const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end,
        const DetourNavigator::RecastSettings& settings, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet);
}

#endif
