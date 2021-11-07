#ifndef OPENMW_MWRENDER_NAVMESH_H
#define OPENMW_MWRENDER_NAVMESH_H

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

        void update(const dtNavMesh& navMesh, const std::size_t number, const std::size_t generation,
                    const std::size_t revision, const DetourNavigator::Settings& settings);

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
        std::size_t mGeneration;
        std::size_t mRevision;
        osg::ref_ptr<osg::Group> mGroup;
    };
}

#endif
