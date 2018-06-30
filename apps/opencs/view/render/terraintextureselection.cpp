#include "terraintextureselection.hpp"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <algorithm>

CSVRender::TerrainTextureSelection::TerrainTextureSelection(osg::Group* parentNode, CSMWorld::CellCoordinates coords, const ESM::Land& esmLand)
    :TerrainSelection (coords, esmLand, parentNode)
{
    osg::ref_ptr<osg::Geometry> geometry {new osg::Geometry{}};

    // Color
    osg::ref_ptr<osg::Vec4Array> colors {new osg::Vec4Array{}};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    geometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    // Primitive
    osg::ref_ptr<osg::DrawArrays> drawArrays {new osg::DrawArrays{osg::PrimitiveSet::Mode::LINES}};

    geometry->addPrimitiveSet(drawArrays);
    geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    mGeode = new osg::Geode{};
    mGeode->addDrawable(geometry);
    getBaseNode()->addChild(mGeode);
}

void CSVRender::TerrainTextureSelection::addToSelection(osg::Vec3d worldPos)
{
    const TerrainPos localPos {toTextureCoords(worldPos)};

    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
        mSelection.push_back(localPos);
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

    const auto geometry = static_cast<const osg::Geometry*>(mGeode->getDrawable(0));
    const osg::ref_ptr<osg::Geometry> newGeometry {
        geometry
        ? new osg::Geometry{*geometry, osg::CopyOp::SHALLOW_COPY}
        : new osg::Geometry{}};

    const osg::ref_ptr<osg::Vec3Array> vertices {new osg::Vec3Array{}};

    for (TerrainPos localPos : mSelection) {
        const int x {localPos.first};
        const int y {localPos.second};

        // check adjacent terrain squares to see if there should be a border between them
        const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
        const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
        const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
        const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));
        int textureSizeToLandSizeModifier = 4;

        if (north == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier)]+5));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier)]+5));
        }

        if (south == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier)]+5));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier)]+5));
        }

        if (east == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier)]+5));
            vertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier)]+5));
        }

        if (west == mSelection.end()) {
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier)]+5));
            vertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x * textureSizeToLandSizeModifier, y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier)]+5));
        }
    }

    newGeometry->setVertexArray(vertices);

    const auto drawArrays = static_cast<osg::DrawArrays*>(newGeometry->getPrimitiveSet(0));

    drawArrays->setCount(vertices->size());
    drawArrays->dirty();

    mGeode->setDrawable(0, newGeometry);
}
