#ifndef OPENMW_COMPONENTS_SCENEUTIL_AGENTPATH_H
#define OPENMW_COMPONENTS_SCENEUTIL_AGENTPATH_H

#include <osg/ref_ptr>

#include <deque>

namespace osg
{
    class Group;
    class Vec3f;
}

namespace DetourNavigator
{
    struct Settings;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createAgentPathGroup(const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end,
            const DetourNavigator::Settings& settings);
}

#endif
