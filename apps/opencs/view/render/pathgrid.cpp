#include "pathgrid.hpp"

#include <set>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>

#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/pathgrid.hpp"

#include "mask.hpp"

namespace CSVRender
{
    const float Pathgrid::PointShapeSize = 50.f;

    Pathgrid::Pathgrid(osg::Group* parent, const CSMWorld::Pathgrid& pathgrid, const CSMWorld::CellCoordinates& coords)
        : mPathgridData(pathgrid)
        , mParentNode(parent)
        , mBaseNode(new osg::PositionAttitudeTransform())
        , mPointGeometry(new osg::Geometry())
        , mEdgeNode(new osg::Geode())
    {
        const int CoordScalar = ESM::Land::REAL_SIZE;

        mParentNode->addChild(mBaseNode);
        mBaseNode->setPosition(osg::Vec3(coords.getX() * CoordScalar, coords.getY() * CoordScalar, 0));
        mBaseNode->setNodeMask(Mask_Pathgrid);
        mBaseNode->addChild(mEdgeNode);

        constructPointShape();
        buildGrid();
    }

    Pathgrid::~Pathgrid()
    {
        destroyGrid();
        mBaseNode->removeChild(mEdgeNode);
        mParentNode->removeChild(mBaseNode);
    }

    bool Pathgrid::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
        const int PointColumn = 3;
        const int EdgeColumn = 4;

        if (topLeft.parent().isValid())
        {
            int start = topLeft.row();
            int end = bottomRight.row();

            if (topLeft.parent().column() == PointColumn)
            {
                std::set<unsigned short> changedEdges;

                for (int row = start; row <= end; ++row)
                {
                    // Recreate point
                    destroyEdges(row);
                    destroyPoint(row);

                    buildPoint(row);
                    changedEdges.insert(row);

                    // Correct affected edges
                    for (unsigned short i = 0; i < mPointData.size(); ++i)
                    {
                        PointData& data = mPointData[i];
                        for (std::vector<unsigned short>::iterator edge = data.edgeList.begin();
                            edge != data.edgeList.end(); ++edge)
                        {
                            if (*edge == row)
                                changedEdges.insert(i);
                        }
                    }
                }

                // Reconstruct edges
                for (std::set<unsigned short>::iterator index = changedEdges.begin();
                    index != changedEdges.end(); ++index)
                {
                    destroyEdges(*index);
                    buildEdges(*index);
                }

                return true;
            }
            else if (topLeft.parent().column() == EdgeColumn)
            {
                const unsigned short NumPoints = static_cast<unsigned short>(mPathgridData.mPoints.size());
                const size_t NumEdges = mPathgridData.mEdges.size();

                std::set<unsigned short> changedEdges;

                // Clear edge lists
                for (int row = start; row <= end; ++row)
                {
                    mPointData[row].edgeList.clear();
                }

                // Recreate edge lists
                for (size_t i = 0; i < NumEdges; ++i)
                {
                    const CSMWorld::Pathgrid::Edge& edge = mPathgridData.mEdges[i];
                    unsigned short v0 = static_cast<unsigned short>(edge.mV0);
                    unsigned short v1 = static_cast<unsigned short>(edge.mV1);

                    if ((v0 >= start && v0 <= end) && (v0 != v1 && v0 < NumPoints && v1 < NumPoints))
                        mPointData[v0].edgeList.push_back(v1);
                }

                // Reconstruct edges
                for (int row = start; row <= end; ++row)
                {
                    destroyEdges(row);
                    buildEdges(row);
                }

                return true;
            }
        }

        return false;
    }

    bool Pathgrid::rowAboutToBeRemoved(const QModelIndex& parent, int start, int end)
    {
        const int PointColumn = 3;
        const int EdgeColumn = 4;

        if (!parent.parent().isValid())
        {
            if (parent.column() == PointColumn)
            {
                // Dangling edges will be removed and edges past start decremented
                for (size_t i = 0; i < mPointData.size(); ++i)
                {
                    PointData& data = mPointData[i];

                    // They are getting removed anyway
                    if (i >= (size_t) start && i <= (size_t) end)
                        continue;

                    bool modified = false;

                    for (std::vector<unsigned short>::iterator edge = data.edgeList.begin();
                        edge != data.edgeList.end();)
                    {
                        if (*edge >= start && *edge <= end)
                        {
                            edge = data.edgeList.erase(edge);
                            modified = true;
                        }
                        else
                        {
                            ++edge;
                        }
                    }

                    // Need to do this before modifying the rest
                    if (modified)
                    {
                        destroyEdges(i);
                        buildEdges(i);
                    }

                    for (std::vector<unsigned short>::iterator edge = data.edgeList.begin();
                        edge != data.edgeList.end(); ++edge)
                    {
                        if (*edge > end)
                            --(*edge);
                    }
                }

                // Remove points
                for (int i = start; i <= end; ++i)
                {
                    destroyEdges(static_cast<unsigned short>(start));
                    destroyPoint(static_cast<unsigned short>(start));
                    mPointData.erase(mPointData.begin() + start);
                }

                return true;
            }
            else if (parent.column() == EdgeColumn)
            {
                std::set<unsigned short> changedEdges;

                // Remove affected edges
                for (int i = start; i <= end; ++i)
                {
                    const CSMWorld::Pathgrid::Edge& edge = mPathgridData.mEdges[i];
                    unsigned short v0 = static_cast<unsigned short>(edge.mV0);
                    unsigned short v1 = static_cast<unsigned short>(edge.mV1);

                    // Only remove one
                    for (std::vector<unsigned short>::iterator it = mPointData[v0].edgeList.begin();
                        it != mPointData[v1].edgeList.end(); ++it)
                    {
                        if (*it == v1)
                        {
                            mPointData[v0].edgeList.erase(it);
                            changedEdges.insert(v0);
                            break;
                        }
                    }
                }

                // Reconstruct edges
                for (std::set<unsigned short>::iterator index = changedEdges.begin();
                    index != changedEdges.end(); ++index)
                {
                    destroyEdges(*index);
                    buildEdges(*index);
                }
            }
        }

        return false;
    }

    bool Pathgrid::rowAdded(const QModelIndex& parent, int start, int end)
    {
        const int PointColumn = 3;
        const int EdgeColumn = 4;

        if (!parent.parent().isValid())
        {
            if (parent.column() == PointColumn)
            {
                // Edges at and beyond start point have been incremented
                for (std::vector<PointData>::iterator point = mPointData.begin(); point != mPointData.end(); ++point)
                {
                    for (std::vector<unsigned short>::iterator edge = point->edgeList.begin();
                        edge != point->edgeList.end(); ++edge)
                    {
                        if (*edge >= start)
                            *edge += end - start + 1;
                    }
                }

                // Add points
                mPointData.insert(mPointData.begin() + start, end - start + 1, PointData());
                for (int row = start; row <= end; ++row)
                    buildPoint(static_cast<unsigned short>(row));

                return true;
            }
            else if (parent.column() == EdgeColumn)
            {
                const unsigned short NumPoints = static_cast<unsigned short>(mPathgridData.mPoints.size());

                std::set<unsigned short> changedEdges;

                // Add edges
                for (int row = start; row <= end; ++row)
                {
                    const CSMWorld::Pathgrid::Edge& edge = mPathgridData.mEdges[row];
                    unsigned short v0 = static_cast<unsigned short>(edge.mV0);
                    unsigned short v1 = static_cast<unsigned short>(edge.mV1);

                    if (v0 != v1 && v0 < NumPoints && v1 < NumPoints)
                    {
                        mPointData[v0].edgeList.push_back(v1);
                        changedEdges.insert(v0);
                    }
                }

                // Reconstruct edges
                for (std::set<unsigned short>::iterator index = changedEdges.begin();
                    index != changedEdges.end(); ++index)
                {
                    destroyEdges(*index);
                    buildEdges(*index);
                }
            }
        }

        return false;
    }

    void Pathgrid::constructPointShape()
    {
        // Construct a diamond
        const unsigned short VertexCount = 24;

        const osg::Vec3f ShapePoints[6] =
        {
            osg::Vec3f( 0.f, 0.f, PointShapeSize),
            osg::Vec3f(-PointShapeSize/2, -PointShapeSize/2,  0.f),
            osg::Vec3f(-PointShapeSize/2,  PointShapeSize/2,  0.f),
            osg::Vec3f( PointShapeSize/2, -PointShapeSize/2,  0.f),
            osg::Vec3f( PointShapeSize/2,  PointShapeSize/2,  0.f),
            osg::Vec3f( 0.f, 0.f, -PointShapeSize)
        };

        const unsigned short ShapeIndices[VertexCount] =
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

        osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec3Array> normalArray = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array();
        osg::ref_ptr<osg::DrawElementsUShort> indexArray = new osg::DrawElementsUShort(
            osg::PrimitiveSet::TRIANGLES, 24);

        for (unsigned short i = 0; i < VertexCount; ++i)
        {
            vertexArray->push_back(ShapePoints[ShapeIndices[i]]);
            indexArray->setElement(i, i);
        }

        for (unsigned short i = 0; i < VertexCount; i += 3)
        {
            osg::Vec3f v1 = vertexArray->at(i+1) - vertexArray->at(i);
            osg::Vec3f v2 = vertexArray->at(i+2) - vertexArray->at(i);
            osg::Vec3f normal = v1 ^ v2;

            normalArray->push_back(normal);
            normalArray->push_back(normal);
            normalArray->push_back(normal);
        }

        colorArray->push_back(osg::Vec4f(1.f, 0.f, 0.f, 1.f));

        mPointGeometry->setVertexArray(vertexArray);
        mPointGeometry->setNormalArray(normalArray, osg::Array::BIND_PER_VERTEX);
        mPointGeometry->setColorArray(colorArray, osg::Array::BIND_OVERALL);
        mPointGeometry->addPrimitiveSet(indexArray);
    }

    void Pathgrid::buildGrid()
    {
        // Note: the number of pathgrid points is limited to the capacity of a signed short
        const unsigned short NumPoints = static_cast<unsigned short>(mPathgridData.mPoints.size());
        const size_t NumEdges = mPathgridData.mEdges.size();

        // Make points
        mPointData.resize(NumPoints);
        for (unsigned short i = 0; i < NumPoints; ++i)
            buildPoint(i);

        // Add edges to list
        for (size_t i = 0; i < NumEdges; ++i)
        {
            const CSMWorld::Pathgrid::Edge& edge = mPathgridData.mEdges[i];
            unsigned short v0 = static_cast<unsigned short>(edge.mV0);
            unsigned short v1 = static_cast<unsigned short>(edge.mV1);

            if (v0 != v1 && v0 < NumPoints && v1 < NumPoints)
                mPointData[v0].edgeList.push_back(v1);
        }

        // Make edges
        for (unsigned short i = 0; i < NumPoints; ++i)
            buildEdges(i);
    }

    void Pathgrid::destroyGrid()
    {
        const unsigned short NumPoints = static_cast<unsigned short>(mPathgridData.mPoints.size());

        for (unsigned short i = 0; i < NumPoints; ++i)
        {
            destroyEdges(i);
            destroyPoint(i);
        }

        mPointData.clear();
    }

    void Pathgrid::buildPoint(unsigned short index)
    {
        const CSMWorld::Pathgrid::Point& source = mPathgridData.mPoints[index];
        PointData& data = mPointData[index];

        data.posNode = new osg::PositionAttitudeTransform();
        data.posNode->setPosition(osg::Vec3f(source.mX, source.mY, source.mZ));
        mBaseNode->addChild(data.posNode);

        osg::ref_ptr<osg::Geode> pointNode = new osg::Geode();
        pointNode->addDrawable(mPointGeometry);
        data.posNode->addChild(pointNode);

        data.edgeGeometry = new osg::Geometry();
        mEdgeNode->addDrawable(data.edgeGeometry);
    }

    void Pathgrid::destroyPoint(unsigned short index)
    {
        PointData& data = mPointData[index];

        mBaseNode->removeChild(data.posNode);
        data.posNode = 0;

        mEdgeNode->removeDrawable(data.edgeGeometry);
        data.edgeGeometry = 0;
    }

    void Pathgrid::buildEdges(unsigned short index)
    {
        // Note: the number of edges per point is limited to the capacity of an unsigned char
        const unsigned short VertexCount = static_cast<unsigned short>(mPointData[index].edgeList.size() * 2);
        const osg::Quat DefaultRotation = osg::Quat(osg::PI / 2.f, osg::Vec3f(0.f, 0.f, 1.f));
        const osg::Vec3f DefaultDirection = osg::Vec3f(0.f, 1.f, 0.f);
        const osg::Vec3f Offset = osg::Vec3f(0.f, 0.f, 10.f);
        const osg::Vec4f StartColor = osg::Vec4f(1.f, 0.f, 0.f, 1.f);
        const osg::Vec4f EndColor = osg::Vec4f(1.f, 0.5f, 0.f, 1.f);

        if (VertexCount == 0)
            return;

        // Construct geometry
        osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array();
        osg::ref_ptr<osg::DrawElementsUShort> indexArray = new osg::DrawElementsUShort(
            osg::PrimitiveSet::LINES, VertexCount);

        for (unsigned short i = 0; i < VertexCount; i += 2)
        {
            const std::vector<unsigned short>& edges = mPointData[index].edgeList;

            const CSMWorld::Pathgrid::Point& originPoint = mPathgridData.mPoints[index];
            const CSMWorld::Pathgrid::Point& destPoint = mPathgridData.mPoints[edges[i / 2]];

            osg::Vec3f origin = osg::Vec3f(originPoint.mX, originPoint.mY, originPoint.mZ);
            osg::Vec3f destination = osg::Vec3f(destPoint.mX, destPoint.mY, destPoint.mZ);
            osg::Vec3f direction = destination - origin;

            direction.z() = 0;
            direction.normalize();

            // In case the x,y coordinates are too similar
            if (direction.isNaN())
                direction = DefaultDirection;

            vertexArray->push_back(origin + Offset + DefaultRotation.inverse() * direction * 0.33f * PointShapeSize);
            vertexArray->push_back(destination+ Offset - DefaultRotation * direction * 0.33f * PointShapeSize);
            colorArray->push_back(StartColor);
            colorArray->push_back(EndColor);
            indexArray->setElement(i, i);
            indexArray->setElement(i + 1, i + 1);
        }

        osg::ref_ptr<osg::Geometry> edgeGeometry = mPointData[index].edgeGeometry;
        edgeGeometry->setVertexArray(vertexArray);
        edgeGeometry->setColorArray(colorArray, osg::Array::BIND_PER_VERTEX);
        edgeGeometry->addPrimitiveSet(indexArray);
        edgeGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    }

    void Pathgrid::destroyEdges(unsigned short index)
    {
        PointData& data = mPointData[index];

        data.edgeGeometry->setVertexArray(0);
        data.edgeGeometry->setColorArray(0);
        data.edgeGeometry->getPrimitiveSetList().clear();
    }
}
