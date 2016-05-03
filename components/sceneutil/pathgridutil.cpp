#include "pathgridutil.hpp"

#include <osg/Geometry>

#include <components/esm/loadland.hpp>
#include <components/esm/loadpgrd.hpp>

namespace SceneUtil
{
    const unsigned short DiamondVertexCount = 6;
    const unsigned short DiamondIndexCount = 24;

    const unsigned short DiamondConnectorVertexCount = 4;

    const unsigned short DiamondTotalVertexCount = DiamondVertexCount + DiamondConnectorVertexCount;

    const float DiamondHalfHeight = 40.f;
    const float DiamondHalfWidth = 16.f;

    const osg::Vec3f DiamondPoints[DiamondVertexCount] =
    {
        osg::Vec3f( 0.f, 0.f, DiamondHalfHeight * 2.f),
        osg::Vec3f(-DiamondHalfWidth, -DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f(-DiamondHalfWidth,  DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( DiamondHalfWidth, -DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( DiamondHalfWidth,  DiamondHalfWidth,  DiamondHalfHeight),
        osg::Vec3f( 0.f, 0.f, 0.f)
    };

    const unsigned short DiamondIndices[DiamondIndexCount] =
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

    const unsigned short DiamondConnectorVertices[DiamondConnectorVertexCount] =
    {
        1, 2, 3, 4
    };

    const osg::Vec4f DiamondColors[DiamondVertexCount] =
    {
        osg::Vec4f(0.f, 0.f, 1.f, 1.f),
        osg::Vec4f(0.f, .05f, .95f, 1.f),
        osg::Vec4f(0.f, .1f, .95f, 1.f),
        osg::Vec4f(0.f, .15f, .95f, 1.f),
        osg::Vec4f(0.f, .2f, .95f, 1.f),
        osg::Vec4f(0.f, .25f, 9.f, 1.f)
    };

    const osg::Vec4f DiamondEdgeColor = osg::Vec4f(0.5f, 1.f, 1.f, 1.f);

    osg::ref_ptr<osg::Geometry> createPathgridGeometry(const ESM::Pathgrid& pathgrid)
    {
        const unsigned short PointCount = static_cast<unsigned short>(pathgrid.mPoints.size());
        const size_t EdgeCount = pathgrid.mEdges.size();

        const unsigned short VertexCount = PointCount * DiamondTotalVertexCount;
        const unsigned short ColorCount = VertexCount;
        const size_t PointIndexCount = PointCount * DiamondIndexCount;
        const size_t EdgeIndexCount = EdgeCount * 2;

        osg::ref_ptr<osg::Geometry> gridGeometry = new osg::Geometry();

        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(VertexCount);
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(ColorCount);
        osg::ref_ptr<osg::DrawElementsUShort> pointIndices =
            new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, PointIndexCount);
        osg::ref_ptr<osg::DrawElementsUShort> lineIndices =
            new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, EdgeIndexCount);

        // Add each point/node
        for (unsigned short pointIndex = 0; pointIndex < PointCount; ++pointIndex)
        {
            const ESM::Pathgrid::Point& point = pathgrid.mPoints[pointIndex];
            osg::Vec3f position = osg::Vec3f(point.mX, point.mY, point.mZ);

            unsigned short vertexOffset = pointIndex * DiamondTotalVertexCount;
            unsigned short indexOffset = pointIndex * DiamondIndexCount;

            // Point
            for (unsigned short i = 0; i < DiamondVertexCount; ++i)
            {
                (*vertices)[vertexOffset + i] = position + DiamondPoints[i];
                (*colors)[vertexOffset + i] = DiamondColors[i];
            }

            for (unsigned short i = 0; i < DiamondIndexCount; ++i)
            {
                pointIndices->setElement(indexOffset + i, vertexOffset + DiamondIndices[i]);
            }

            // Connectors
            vertexOffset += DiamondVertexCount;
            for (unsigned short i = 0; i < DiamondConnectorVertexCount; ++i)
            {
                (*vertices)[vertexOffset + i] = position + DiamondPoints[DiamondConnectorVertices[i]];
                (*colors)[vertexOffset + i] = DiamondEdgeColor;
            }
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
                diamondIndex = 0;
            else if (dir.y() >= 0 && dir.x() > 0)
                diamondIndex = 3;
            else if (dir.x() <= 0 && dir.y() > 0)
                diamondIndex = 1;
            else if (dir.y() <= 0 && dir.x() < 0)
                diamondIndex = 0;
            else if (dir.x() >= 0 && dir.y() < 0)
                diamondIndex = 2;

            unsigned short fromIndex = static_cast<unsigned short>(edge->mV0);
            unsigned short toIndex = static_cast<unsigned short>(edge->mV1);

            lineIndices->setElement(lineIndex++, fromIndex * DiamondTotalVertexCount + DiamondVertexCount + diamondIndex);
            lineIndices->setElement(lineIndex++, toIndex * DiamondTotalVertexCount + DiamondVertexCount + diamondIndex);
        }

        lineIndices->resize(lineIndex);

        gridGeometry->setVertexArray(vertices);
        gridGeometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
        gridGeometry->addPrimitiveSet(pointIndices);
        gridGeometry->addPrimitiveSet(lineIndices);
        gridGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        return gridGeometry;
    }
}
