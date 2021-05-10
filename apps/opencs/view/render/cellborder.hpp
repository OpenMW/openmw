#ifndef OPENCS_VIEW_CELLBORDER_H
#define OPENCS_VIEW_CELLBORDER_H

#include <cstddef>

#include <osg/ref_ptr>

namespace osg
{
    class Geometry;
    class Group;
    class PositionAttitudeTransform;
}

namespace ESM
{
    struct Land;
}

namespace CSMWorld
{
    class CellCoordinates;
}

namespace CSVRender
{

    class CellBorder
    {
    public:

        CellBorder(osg::Group* cellNode, const CSMWorld::CellCoordinates& coords);
        ~CellBorder();

        void buildShape(const ESM::Land& esmLand);

    private:

        static const int CellSize;
        static const int VertexCount;

        size_t landIndex(int x, int y);
        float scaleToWorld(int val);

        // unimplemented
        CellBorder(const CellBorder&);
        CellBorder& operator=(const CellBorder&);

        osg::Group* mParentNode;
        osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
        osg::ref_ptr<osg::Geometry> mBorderGeometry;
    };
}

#endif
