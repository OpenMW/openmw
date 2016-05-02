#ifndef OPENCS_VIEW_PATHGRID_H
#define OPENCS_VIEW_PATHGRID_H

#include <vector>

#include <osg/ref_ptr>
#include <QModelIndex>

namespace osg
{
    class Geode;
    class Geometry;
    class Group;
    class PositionAttitudeTransform;
}

namespace CSMWorld
{
    struct Pathgrid;
    class CellCoordinates;
}

namespace CSVRender
{
    class Pathgrid
    {
        public:

            Pathgrid(osg::Group* parent, const CSMWorld::Pathgrid& pathgrid, const CSMWorld::CellCoordinates& coords);
            ~Pathgrid();

            bool dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

            bool rowAboutToBeRemoved(const QModelIndex& parent, int start, int end);

            bool rowAdded(const QModelIndex& parent, int start, int end);

        private:

            struct PointData
            {
                osg::ref_ptr<osg::PositionAttitudeTransform> posNode;
                std::vector<unsigned short> edgeList;
                osg::ref_ptr<osg::Geometry> edgeGeometry;
            };

            static const float PointShapeSize;

            const CSMWorld::Pathgrid& mPathgridData;
            std::vector<PointData> mPointData;

            osg::ref_ptr<osg::Group> mParentNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            osg::ref_ptr<osg::Geometry> mPointGeometry;
            osg::ref_ptr<osg::Geode> mEdgeNode;

            void constructPointShape();

            void buildGrid();
            void destroyGrid();

            void buildPoint(unsigned short index);
            void destroyPoint(unsigned short index);

            void buildEdges(unsigned short index);
            void destroyEdges(unsigned short index);

            // Not implemented
            Pathgrid(const Pathgrid&);
            Pathgrid& operator=(const Pathgrid&);
    };
}

#endif
