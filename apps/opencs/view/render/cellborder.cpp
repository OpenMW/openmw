#include "cellborder.hpp"

#include <osg/Array>
#include <osg/GL>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/PrimitiveSet>
#include <osg/StateAttribute>
#include <osg/StateSet>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/esm3/loadland.hpp>

#include "mask.hpp"

#include "../../model/world/cellcoordinates.hpp"

namespace
{
    constexpr int CellSize = ESM::Land::REAL_SIZE;

    /*
        The number of vertices per cell border is equal to the number of vertices per edge
        minus the duplicated corner vertices. An additional vertex to close the loop is NOT needed.
    */
    constexpr unsigned VertexCount = (ESM::Land::LAND_SIZE * 4) - 4;
}

CSVRender::CellBorder::CellBorder(osg::Group* cellNode, const CSMWorld::CellCoordinates& coords)
    : mParentNode(cellNode)
{
    mBorderGeometry = new osg::Geometry();

    mBaseNode = new osg::PositionAttitudeTransform();
    mBaseNode->setNodeMask(Mask_CellBorder);
    mBaseNode->setPosition(osg::Vec3f(coords.getX() * CellSize, coords.getY() * CellSize, 10));
    mBaseNode->addChild(mBorderGeometry);

    mParentNode->addChild(mBaseNode);
}

CSVRender::CellBorder::~CellBorder()
{
    mParentNode->removeChild(mBaseNode);
}

void CSVRender::CellBorder::buildShape(const ESM::Land& esmLand)
{
    const ESM::Land::LandData* landData = esmLand.getLandData(ESM::Land::DATA_VHGT);

    mBaseNode->removeChild(mBorderGeometry);
    mBorderGeometry = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

    int x = 0;
    int y = 0;

    /*
        Traverse the cell border counter-clockwise starting at the SW corner vertex (0, 0).
        Each loop starts at a corner vertex and ends right before the next corner vertex.
    */
    if (landData)
    {
        for (; x < ESM::Land::LAND_SIZE - 1; ++x)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

        x = ESM::Land::LAND_SIZE - 1;
        for (; y < ESM::Land::LAND_SIZE - 1; ++y)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

        y = ESM::Land::LAND_SIZE - 1;
        for (; x > 0; --x)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

        x = 0;
        for (; y > 0; --y)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));
    }
    else
    {
        for (; x < ESM::Land::LAND_SIZE - 1; ++x)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), ESM::Land::DEFAULT_HEIGHT));

        x = ESM::Land::LAND_SIZE - 1;
        for (; y < ESM::Land::LAND_SIZE - 1; ++y)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), ESM::Land::DEFAULT_HEIGHT));

        y = ESM::Land::LAND_SIZE - 1;
        for (; x > 0; --x)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), ESM::Land::DEFAULT_HEIGHT));

        x = 0;
        for (; y > 0; --y)
            vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), ESM::Land::DEFAULT_HEIGHT));
    }

    mBorderGeometry->setVertexArray(vertices);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    mBorderGeometry->setColorArray(colors, osg::Array::BIND_PER_PRIMITIVE_SET);

    osg::ref_ptr<osg::DrawElementsUShort> primitives
        = new osg::DrawElementsUShort(osg::PrimitiveSet::LINE_STRIP, VertexCount + 1);

    // Assign one primitive to each vertex.
    for (unsigned i = 0; i < VertexCount; ++i)
        primitives->setElement(i, i);

    // Assign the last primitive to the first vertex to close the loop.
    primitives->setElement(VertexCount, 0);

    mBorderGeometry->addPrimitiveSet(primitives);
    mBorderGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    mBaseNode->addChild(mBorderGeometry);
}

size_t CSVRender::CellBorder::landIndex(int x, int y)
{
    return static_cast<size_t>(y) * ESM::Land::LAND_SIZE + x;
}

float CSVRender::CellBorder::scaleToWorld(int value)
{
    return (CellSize + 128) * (float)value / ESM::Land::LAND_SIZE;
}
