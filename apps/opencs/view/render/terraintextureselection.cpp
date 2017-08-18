#include "terraintextureselection.hpp"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

CSVRender::TerrainTextureSelection::TerrainTextureSelection(osg::Group* parentNode, const CSMWorld::CellCoordinates& coords, const ESM::Land& esmLand)
    :TerrainSelection {coords, esmLand, parentNode}, mGeometry {new osg::Geometry()}
{
    // Vertices
    osg::ref_ptr<osg::Vec3Array> vertices {new osg::Vec3Array()};
    mGeometry->setVertexArray(vertices);

    // Color
    osg::ref_ptr<osg::Vec4Array> colors {new osg::Vec4Array()};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    mGeometry->setColorArray(colors, osg::Array::BIND_PER_PRIMITIVE_SET);

    mGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Geode> geode {new osg::Geode()};
    geode->addDrawable(mGeometry);
    getBaseNode()->addChild(geode);

    // Primitive
    osg::ref_ptr<osg::DrawElementsUShort> primitives
    {new osg::DrawElementsUShort(osg::PrimitiveSet::LINE_STRIP, 0)};

    mGeometry->addPrimitiveSet(primitives);
}

void CSVRender::TerrainTextureSelection::addToSelection(osg::Vec3d worldPos)
{
    const TerrainPos localPos {toTextureCoords(worldPos)};

    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
        mSelection.push_back(localPos);

    // TODO: fix this to use std::lower_bound for a binary search
}

void CSVRender::TerrainTextureSelection::toggleSelection(osg::Vec3d worldPos)
{
    const TerrainPos localPos {toTextureCoords(worldPos)};
    const auto iter = std::find(mSelection.begin(), mSelection.end(), localPos);

    if (iter != mSelection.end()) {
        mSelection.erase(iter);
    }
    else {
        mSelection.push_back(localPos);
    }
}

void CSVRender::TerrainTextureSelection::deselect()
{
    mSelection.clear();
}

void CSVRender::TerrainTextureSelection::update()
{
    const ESM::Land::LandData* landData {getLandData()};

    if (!landData)
        return;

    auto vertices = static_cast<osg::Vec3Array*>(mGeometry->getVertexArray());

    for (TerrainPos localPos : mSelection) {
        const int x {localPos.first};
        const int y {localPos.second};

        // check adjacent terrain squares to see if there should be a border between them
        const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
        const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
        const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
        const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));

        if (north == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x, y + 1)]));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x + 1, y + 1)]));
        }

        if (south == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x, y)]));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x + 1, y)]));
        }

        if (east == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x + 1, y)]));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x + 1, y + 1)]));
        }

        if (west == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x, y)]));
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x, y + 1)]));
        }
    }

    auto primitives = static_cast<osg::DrawElementsUShort*>(mGeometry->getPrimitiveSet(0));

    for (size_t i = 0; i != vertices->size(); i += 2)
        primitives->setElement(i, i + 1);

    auto drawArrays = static_cast<osg::DrawArrays*>(mGeometry->getPrimitiveSet(0));
    drawArrays->setCount(vertices->size());
    drawArrays->dirty();
}
