#ifndef OPENMW_MWRENDER_AGENTSPATHS_H
#define OPENMW_MWRENDER_AGENTSPATHS_H

#include "apps/openmw/mwworld/ptr.hpp"

#include <osg/ref_ptr>

#include <deque>
#include <map>

namespace osg
{
    class Group;
    class StateSet;
}

namespace DetourNavigator
{
    struct Settings;
    struct AgentBounds;
}

namespace MWRender
{
    class ActorsPaths
    {
    public:
        ActorsPaths(const osg::ref_ptr<osg::Group>& root, bool enabled);
        ~ActorsPaths();

        bool toggle();

        void update(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end,
            const DetourNavigator::Settings& settings);

        void remove(const MWWorld::ConstPtr& actor);

        void removeCell(const MWWorld::CellStore* const store);

        void updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated);

        void enable();

        void disable();

    private:
        struct Group
        {
            const MWWorld::CellStore* mCell;
            osg::ref_ptr<osg::Group> mNode;
        };

        using Groups = std::map<const MWWorld::LiveCellRefBase*, Group>;

        osg::ref_ptr<osg::Group> mRootNode;
        Groups mGroups;
        bool mEnabled;
        osg::ref_ptr<osg::StateSet> mGroupStateSet;
        osg::ref_ptr<osg::StateSet> mDebugDrawStateSet;
    };
}

#endif
