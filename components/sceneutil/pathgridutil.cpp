#include "pathgridutil.hpp"

#include <osg/Geometry>
#include <osg/Material>

#include <components/esm3/loadpgrd.hpp>

namespace SceneUtil
{
    namespace
    {
        constexpr unsigned short DiamondVertexCount = 6;
        constexpr unsigned short DiamondIndexCount = 24;
        constexpr unsigned short DiamondWireframeIndexCount = 24;

        constexpr unsigned short DiamondConnectorVertexCount = 4;

        constexpr unsigned short DiamondTotalVertexCount = DiamondVertexCount + DiamondConnectorVertexCount;

        constexpr float DiamondWireframeScalar = 1.1f;

        const osg::Vec3f DiamondPoints[DiamondVertexCount] = { osg::Vec3f(0.f, 0.f, DiamondHalfHeight * 2.f),
            osg::Vec3f(-DiamondHalfWidth, -DiamondHalfWidth, DiamondHalfHeight),
            osg::Vec3f(-DiamondHalfWidth, DiamondHalfWidth, DiamondHalfHeight),
            osg::Vec3f(DiamondHalfWidth, -DiamondHalfWidth, DiamondHalfHeight),
            osg::Vec3f(DiamondHalfWidth, DiamondHalfWidth, DiamondHalfHeight), osg::Vec3f(0.f, 0.f, 0.f) };

        constexpr unsigned short DiamondIndices[DiamondIndexCount]
            = { 0, 2, 1, 0, 1, 3, 0, 3, 4, 0, 4, 2, 5, 1, 2, 5, 3, 1, 5, 4, 3, 5, 2, 4 };

        constexpr unsigned short DiamondWireframeIndices[DiamondWireframeIndexCount]
            = { 0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1, 5, 1, 5, 2, 5, 3, 5, 4 };

        constexpr unsigned short DiamondConnectorVertices[DiamondConnectorVertexCount] = { 1, 2, 3, 4 };

        const osg::Vec4f DiamondColors[DiamondVertexCount]
            = { osg::Vec4f(0.f, 0.f, 1.f, 1.f), osg::Vec4f(0.f, .05f, .95f, 1.f), osg::Vec4f(0.f, .1f, .95f, 1.f),
                  osg::Vec4f(0.f, .15f, .95f, 1.f), osg::Vec4f(0.f, .2f, .95f, 1.f), osg::Vec4f(0.f, .25f, 9.f, 1.f) };

        const osg::Vec4f DiamondEdgeColor = osg::Vec4f(0.5f, 1.f, 1.f, 1.f);
        const osg::Vec4f DiamondWireColor = osg::Vec4f(0.72f, 0.f, 0.96f, 1.f);
        const osg::Vec4f DiamondFocusWireColor = osg::Vec4f(0.91f, 0.66f, 1.f, 1.f);
    }

    osg::ref_ptr<osg::Geometry> createPathgridGeometry(const ESM::Pathgrid& pathgrid)
    {
        const size_t vertexCount = pathgrid.mPoints.size() * DiamondTotalVertexCount;
        const size_t pointIndexCount = pathgrid.mPoints.size() * DiamondIndexCount;
        const size_t edgeIndexCount = pathgrid.mEdges.size() * 2;

        osg::ref_ptr<osg::Geometry> gridGeometry = new osg::Geometry();

        if (pointIndexCount || edgeIndexCount)
        {
            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(vertexCount);
            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(vertexCount);
            osg::ref_ptr<osg::DrawElementsUInt> pointIndices
                = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, pointIndexCount);
            osg::ref_ptr<osg::DrawElementsUInt> lineIndices
                = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, edgeIndexCount);

            // Add each point/node
            for (size_t pointIndex = 0; pointIndex < pathgrid.mPoints.size(); ++pointIndex)
            {
                const ESM::Pathgrid::Point& point = pathgrid.mPoints[pointIndex];
                osg::Vec3f position = osg::Vec3f(point.mX, point.mY, point.mZ);

                size_t vertexOffset = pointIndex * DiamondTotalVertexCount;
                size_t indexOffset = pointIndex * DiamondIndexCount;

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
            unsigned int lineIndex = 0;

            for (const ESM::Pathgrid::Edge& edge : pathgrid.mEdges)
            {
                if (edge.mV0 == edge.mV1 || edge.mV0 >= pathgrid.mPoints.size() || edge.mV1 >= pathgrid.mPoints.size())
                    continue;

                const ESM::Pathgrid::Point& from = pathgrid.mPoints[edge.mV0];
                const ESM::Pathgrid::Point& to = pathgrid.mPoints[edge.mV1];

                osg::Vec3f fromPos = osg::Vec3f(from.mX, from.mY, from.mZ);
                osg::Vec3f toPos = osg::Vec3f(to.mX, to.mY, to.mZ);
                osg::Vec3f dir = toPos - fromPos;
                dir.normalize();

                osg::Quat rot(static_cast<float>(-osg::PI_2), osg::Vec3f(0, 0, 1));
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

                unsigned fromIndex = static_cast<unsigned>(edge.mV0);
                unsigned toIndex = static_cast<unsigned>(edge.mV1);

                lineIndices->setElement(
                    lineIndex++, fromIndex * DiamondTotalVertexCount + DiamondVertexCount + diamondIndex);
                lineIndices->setElement(
                    lineIndex++, toIndex * DiamondTotalVertexCount + DiamondVertexCount + diamondIndex);
            }

            lineIndices->resize(lineIndex);

            gridGeometry->setVertexArray(vertices);
            gridGeometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
            if (pointIndexCount)
                gridGeometry->addPrimitiveSet(pointIndices);
            if (edgeIndexCount)
                gridGeometry->addPrimitiveSet(lineIndices);
            gridGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        }

        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        gridGeometry->getOrCreateStateSet()->setAttribute(material);

        return gridGeometry;
    }

    osg::ref_ptr<osg::Geometry> createPathgridSelectedWireframe(
        const ESM::Pathgrid& pathgrid, const std::vector<unsigned short>& selected)
    {
        const size_t vertexCount = selected.size() * DiamondVertexCount;
        const size_t indexCount = selected.size() * DiamondWireframeIndexCount;

        osg::ref_ptr<osg::Geometry> wireframeGeometry = new osg::Geometry();

        if (indexCount)
        {
            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(vertexCount);
            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(vertexCount);
            osg::ref_ptr<osg::DrawElementsUInt> indices
                = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, indexCount);

            osg::Vec3f wireOffset = osg::Vec3f(0, 0, (1 - DiamondWireframeScalar) * DiamondHalfHeight);

            // Add each point/node
            for (size_t it = 0; it < selected.size(); ++it)
            {
                const ESM::Pathgrid::Point& point = pathgrid.mPoints[selected[it]];
                osg::Vec3f position = osg::Vec3f(point.mX, point.mY, point.mZ) + wireOffset;

                size_t vertexOffset = it * DiamondVertexCount;
                size_t indexOffset = it * DiamondWireframeIndexCount;

                // Point
                for (unsigned short i = 0; i < DiamondVertexCount; ++i)
                {
                    (*vertices)[vertexOffset + i] = position + DiamondPoints[i] * DiamondWireframeScalar;

                    if (it == selected.size() - 1)
                        (*colors)[vertexOffset + i] = DiamondFocusWireColor;
                    else
                        (*colors)[vertexOffset + i] = DiamondWireColor;
                }

                for (unsigned short i = 0; i < DiamondWireframeIndexCount; ++i)
                {
                    indices->setElement(indexOffset + i, vertexOffset + DiamondWireframeIndices[i]);
                }
            }

            wireframeGeometry->setVertexArray(vertices);
            wireframeGeometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
            wireframeGeometry->addPrimitiveSet(indices);
            wireframeGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        }
        return wireframeGeometry;
    }

    unsigned short getPathgridNode(unsigned vertexIndex)
    {
        return static_cast<unsigned short>(vertexIndex / (DiamondVertexCount + DiamondConnectorVertexCount));
    }
}
