#ifndef OPENMW_MWRENDER_NAVMESH_H
#define OPENMW_MWRENDER_NAVMESH_H

#include <components/detournavigator/version.hpp>
#include <components/detournavigator/tileposition.hpp>

#include <osg/ref_ptr>

#include <cstddef>
#include <map>

class dtNavMesh;

namespace osg
{
    class Group;
    class Geometry;
    class StateSet;
}

namespace DetourNavigator
{
    class NavMeshCacheItem;
    struct Settings;
}

namespace MWRender
{
    class NavMesh
    {
    public:
        NavMesh(const osg::ref_ptr<osg::Group>& root, bool enabled);
        ~NavMesh();

        bool toggle();

        void update(const DetourNavigator::NavMeshCacheItem& navMesh, std::size_t id,
            const DetourNavigator::Settings& settings);

        void reset();

        void enable();

        void disable();

        bool isEnabled() const
        {
            return mEnabled;
        }

    private:
        struct Tile
        {
            DetourNavigator::Version mVersion;
            osg::ref_ptr<osg::Group> mGroup;
        };

        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::StateSet> mGroupStateSet;
        osg::ref_ptr<osg::StateSet> mDebugDrawStateSet;
        bool mEnabled;
        std::size_t mId;
        DetourNavigator::Version mVersion;
        std::map<DetourNavigator::TilePosition, Tile> mTiles;
    };
}

#endif
