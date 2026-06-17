#ifndef OPENMW_MWRENDER_RECASTMESH_H
#define OPENMW_MWRENDER_RECASTMESH_H

#include <components/detournavigator/recastmeshtiles.hpp>
#include <components/detournavigator/version.hpp>

#include <osg/ref_ptr>

namespace osg
{
    class Group;
    class Geometry;
    class StateSet;
}

namespace DetourNavigator
{
    struct Settings;
}

namespace MWRender
{
    class RecastMesh
    {
    public:
        RecastMesh(const osg::ref_ptr<osg::Group>& root, bool enabled);
        ~RecastMesh();

        bool toggle();

        void update(const DetourNavigator::RecastMeshTiles& recastMeshTiles, const DetourNavigator::Settings& settings);

        void reset();

        void enable();

        void disable();

        bool isEnabled() const { return mEnabled; }

    private:
        struct Group
        {
            DetourNavigator::Version mVersion;
            osg::ref_ptr<osg::Group> mValue;
        };

        osg::ref_ptr<osg::Group> mRootNode;
        bool mEnabled;
        std::map<DetourNavigator::TilePosition, Group> mGroups;
        osg::ref_ptr<osg::StateSet> mGroupStateSet;
        osg::ref_ptr<osg::StateSet> mDebugDrawStateSet;
    };
}

#endif
