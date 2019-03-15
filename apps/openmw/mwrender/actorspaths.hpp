#ifndef OPENMW_MWRENDER_AGENTSPATHS_H
#define OPENMW_MWRENDER_AGENTSPATHS_H

#include <apps/openmw/mwworld/ptr.hpp>

#include <components/detournavigator/navigator.hpp>

#include <osg/ref_ptr>

#include <unordered_map>
#include <deque>

namespace osg
{
    class Group;
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
                const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end,
                const DetourNavigator::Settings& settings);

        void remove(const MWWorld::ConstPtr& actor);

        void removeCell(const MWWorld::CellStore* const store);

        void updatePtr(const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& updated);

        void enable();

        void disable();

    private:
        using Groups = std::map<MWWorld::ConstPtr, osg::ref_ptr<osg::Group>>;

        osg::ref_ptr<osg::Group> mRootNode;
        Groups mGroups;
        bool mEnabled;
    };
}

#endif
