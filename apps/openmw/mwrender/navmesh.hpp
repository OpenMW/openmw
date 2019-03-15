#ifndef OPENMW_MWRENDER_NAVMESH_H
#define OPENMW_MWRENDER_NAVMESH_H

#include <components/detournavigator/navigator.hpp>

#include <osg/ref_ptr>

namespace osg
{
    class Group;
    class Geometry;
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
        std::size_t mId = std::numeric_limits<std::size_t>::max();
        std::size_t mGeneration;
        std::size_t mRevision;
        osg::ref_ptr<osg::Group> mGroup;
    };
}

#endif
