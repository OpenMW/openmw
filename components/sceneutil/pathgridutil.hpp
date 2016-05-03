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

            void generateNormals();

            void addPoint(unsigned short offset, const osg::Vec3f& position, osg::Vec3Array* vertices,
                osg::Vec3Array* normals, osg::DrawElementsUShort* indices);

            // Not implemented
            PathgridGeometryFactory(const PathgridGeometryFactory&);
            PathgridGeometryFactory& operator=(const PathgridGeometryFactory&);

            std::vector<osg::Vec3f> mGeneratedNormals;
    };
}

#endif
