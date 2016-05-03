#ifndef OPENMW_COMPONENTS_PATHGRIDUTIL_H
#define OPENMW_COMPONENTS_PATHGRIDUTIL_H

#include <osg/ref_ptr>
#include <osg/Geometry>

namespace ESM
{
    class Pathgrid;
}

namespace SceneUtil
{

    class PathgridGeometryFactory
    {
        public:

            osg::ref_ptr<osg::Geometry> create(const ESM::Pathgrid& pathgrid);
            static PathgridGeometryFactory& get();

        private:

            PathgridGeometryFactory();

            // Not implemented
            PathgridGeometryFactory(const PathgridGeometryFactory&);
            PathgridGeometryFactory& operator=(const PathgridGeometryFactory&);
    };
}

#endif
