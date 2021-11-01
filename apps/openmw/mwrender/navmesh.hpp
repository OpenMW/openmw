#ifndef OPENMW_MWRENDER_NAVMESH_H
#define OPENMW_MWRENDER_NAVMESH_H

#include <components/detournavigator/version.hpp>

#include <osg/ref_ptr>

#include <cstddef>

class dtNavMesh;

namespace osg
{
    class Group;
    class Geometry;
}

namespace DetourNavigator
{
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

        void update(const dtNavMesh& navMesh, std::size_t id, const DetourNavigator::Version& version,
            const DetourNavigator::Settings& settings);

        void reset();

        void enable();

        void disable();

        bool isEnabled() const
        {
            return mEnabled;
        }

    private:
        osg::ref_ptr<osg::Group> mRootNode;
        bool mEnabled;
        std::size_t mId;
        DetourNavigator::Version mVersion;
        osg::ref_ptr<osg::Group> mGroup;
    };
}

#endif
