#include "pathgridutil.hpp"

#include <osg/Geometry>

#include <components/esm/loadland.hpp>
#include <components/esm/loadpgrd.hpp>

namespace SceneUtil
{
    const unsigned short DiamondVertexCount = 24;
    const float DiamondHalfHeight = 25.f;
    const float DiamondHalfWidth = 10.f;

    const osg::Vec3f DiamondPoints[6] =
    {
        osg::Vec3f( 0.f, 0.f, DiamondHalfHeight * 2.f),
        osg::Vec3f(-DiamondHalfWidth, -DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f(-DiamondHalfWidth,  DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( DiamondHalfWidth, -DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( DiamondHalfWidth,  DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( 0.f, 0.f, 0.f)
    };

    const unsigned short DiamondIndices[DiamondVertexCount] =
    {
        0, 2, 1,
        0, 1, 3,
        0, 3, 4,
        0, 4, 2,
        5, 1, 2,
        5, 3, 1,
        5, 4, 3,
        5, 2, 4
    };

    const osg::Vec4f DiamondColor = osg::Vec4f(1.f, 0.f, 0.f, 1.f);

    osg::ref_ptr<osg::Geometry> PathgridGeometryFactory::create(const ESM::Pathgrid& pathgrid)
    {
        const unsigned short PointCount = static_cast<unsigned short>(pathgrid.mPoints.size());
        const size_t EdgeCount = pathgrid.mEdges.size();

        const unsigned short VertexCount = PointCount * DiamondVertexCount;

        osg::ref_ptr<osg::Geometry> gridGeometry = new osg::Geometry();

        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(VertexCount);
        osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array(VertexCount);
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
        osg::ref_ptr<osg::DrawElementsUShort> pointIndices =
            new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, VertexCount);
        osg::ref_ptr<osg::DrawElementsUShort> lineIndices =
            new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, EdgeCount * 2);

        // Add each point/node
        for (unsigned short i = 0; i < PointCount; ++i)
        {
            const ESM::Pathgrid::Point& point = pathgrid.mPoints[i];
            osg::Vec3f position = osg::Vec3f(point.mX, point.mY, point.mZ);

            addPoint(i * DiamondVertexCount, position, vertices, normals, pointIndices);
        }

        // Add edges
        unsigned short lineIndex = 0;

        for (ESM::Pathgrid::EdgeList::const_iterator edge = pathgrid.mEdges.begin();
            edge != pathgrid.mEdges.end(); ++edge)
        {
            if (edge->mV0 == edge->mV1 || edge->mV0 < 0 || edge->mV0 >= PointCount ||
                edge->mV1 < 0 || edge->mV1 >= PointCount)
                continue;

            const ESM::Pathgrid::Point& from = pathgrid.mPoints[edge->mV0];
            const ESM::Pathgrid::Point& to = pathgrid.mPoints[edge->mV1];

            osg::Vec3f fromPos = osg::Vec3f(from.mX, from.mY, from.mZ);
            osg::Vec3f toPos = osg::Vec3f(to.mX, to.mY, to.mZ);
            osg::Vec3f dir = toPos - fromPos;
            dir.normalize();

            osg::Quat rot = osg::Quat(-osg::PI / 2, osg::Vec3(0, 0, 1));
            dir = rot * dir;

            unsigned short diamondIndex = 0;
            if (dir.isNaN())
                diamondIndex = 2;
            else if (dir.y() >= 0 && dir.x() > 0)
                diamondIndex = 8;
            else if (dir.x() <= 0 && dir.y() > 0)
                diamondIndex = 11;
            else if (dir.y() <= 0 && dir.x() < 0)
                diamondIndex = 2;
            else if (dir.x() >= 0 && dir.y() < 0)
                diamondIndex = 5;

            unsigned short fromIndex = static_cast<unsigned short>(edge->mV0);
            unsigned short toIndex = static_cast<unsigned short>(edge->mV1);

            lineIndices->setElement(lineIndex++, fromIndex * DiamondVertexCount + diamondIndex);
            lineIndices->setElement(lineIndex++, toIndex * DiamondVertexCount + diamondIndex);
        }

        lineIndices->resize(lineIndex);

        (*colors)[0] = DiamondColor;

        gridGeometry->setVertexArray(vertices);
        gridGeometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
        gridGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);
        gridGeometry->addPrimitiveSet(pointIndices);
        gridGeometry->addPrimitiveSet(lineIndices);

        return gridGeometry;
    }

    PathgridGeometryFactory& PathgridGeometryFactory::get()
    {
        static PathgridGeometryFactory instance;
        return instance;
    }

    PathgridGeometryFactory::PathgridGeometryFactory()
    {
        generateNormals();
    }

    void PathgridGeometryFactory::generateNormals()
    {
        mGeneratedNormals.resize(DiamondVertexCount);

        for (unsigned short i = 0; i < DiamondVertexCount; i += 3)
        {
            osg::Vec3f v1 = DiamondPoints[DiamondIndices[i + 1]] - DiamondPoints[DiamondIndices[i]];
            osg::Vec3f v2 = DiamondPoints[DiamondIndices[i + 2]] - DiamondPoints[DiamondIndices[i]];

            osg::Vec3f normal = v1 ^ v2;

            mGeneratedNormals[i] = normal;
            mGeneratedNormals[i + 1] = normal;
            mGeneratedNormals[i + 2] = normal;
        }
    }

    void PathgridGeometryFactory::addPoint(unsigned short offset, const osg::Vec3f& position, osg::Vec3Array* vertices,
                osg::Vec3Array* normals, osg::DrawElementsUShort* indices)
    {
        for (unsigned short i = 0; i < DiamondVertexCount; ++i)
        {
            (*vertices)[i + offset] = position + DiamondPoints[DiamondIndices[i]];
            (*normals)[i + offset] = mGeneratedNormals[i];
            indices->setElement(i + offset, i + offset);
        }
    }
}
