#include "cellborder.hpp"

#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <components/esm/loadland.hpp>
#include <components/sceneutil/vismask.hpp>

#include "../../model/world/cellcoordinates.hpp"

const int CSVRender::CellBorder::CellSize = ESM::Land::REAL_SIZE;
const int CSVRender::CellBorder::VertexCount = (ESM::Land::LAND_SIZE * 4) - 3;


CSVRender::CellBorder::CellBorder(osg::Group* cellNode, const CSMWorld::CellCoordinates& coords)
    : mParentNode(cellNode)
{
    mBaseNode = new osg::PositionAttitudeTransform();
    mBaseNode->setNodeMask(SceneUtil::Mask_EditorCellBorder);
    mBaseNode->setPosition(osg::Vec3f(coords.getX() * CellSize, coords.getY() * CellSize, 10));

    mParentNode->addChild(mBaseNode);
}

CSVRender::CellBorder::~CellBorder()
{
    mParentNode->removeChild(mBaseNode);
}

void CSVRender::CellBorder::buildShape(const ESM::Land& esmLand)
{
    const ESM::Land::LandData* landData = esmLand.getLandData(ESM::Land::DATA_VHGT);

    if (!landData)
        return;

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // Vertices
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

    int x = 0, y = 0;
    for (; x < ESM::Land::LAND_SIZE; ++x)
        vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

    x = ESM::Land::LAND_SIZE - 1;
    for (; y < ESM::Land::LAND_SIZE; ++y)
        vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

    y = ESM::Land::LAND_SIZE - 1;
    for (; x >= 0; --x)
        vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

    x = 0;
    for (; y >= 0; --y)
        vertices->push_back(osg::Vec3f(scaleToWorld(x), scaleToWorld(y), landData->mHeights[landIndex(x, y)]));

    geometry->setVertexArray(vertices);

    // Color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    geometry->setColorArray(colors, osg::Array::BIND_PER_PRIMITIVE_SET);

    // Primitive
    osg::ref_ptr<osg::DrawElementsUShort> primitives =
        new osg::DrawElementsUShort(osg::PrimitiveSet::LINE_STRIP, VertexCount+1);

    for (size_t i = 0; i < VertexCount; ++i)
        primitives->setElement(i, i);

    primitives->setElement(VertexCount, 0);

    geometry->addPrimitiveSet(primitives);
    geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);


    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry);
    mBaseNode->addChild(geode);
}

size_t CSVRender::CellBorder::landIndex(int x, int y)
{
    return y * ESM::Land::LAND_SIZE + x;
}

float CSVRender::CellBorder::scaleToWorld(int value)
{
    return (CellSize + 128) * (float)value / ESM::Land::LAND_SIZE;
}
