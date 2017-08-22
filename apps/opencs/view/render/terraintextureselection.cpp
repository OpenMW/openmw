#include "terraintextureselection.hpp"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

CSVRender::TerrainTextureSelection::TerrainTextureSelection(osg::Group* parentNode, const CSMWorld::CellCoordinates& coords, const ESM::Land& esmLand)
    :TerrainSelection {coords, esmLand, parentNode}
{
    osg::ref_ptr<osg::Geometry> geometry {new osg::Geometry{}};

    // Vertices
    mVertices = new osg::Vec3Array{};
    geometry->setVertexArray(mVertices);

    // Color
    osg::ref_ptr<osg::Vec4Array> colors {new osg::Vec4Array{}};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    geometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    // Primitive
    mDrawArrays = new osg::DrawArrays{osg::PrimitiveSet::Mode::LINES};

    geometry->addPrimitiveSet(mDrawArrays);
    geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    osg::ref_ptr<osg::Geode> geode {new osg::Geode{}};
    geode->addDrawable(geometry);
    getBaseNode()->addChild(geode);
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

    mVertices->clear();

    for (TerrainPos localPos : mSelection) {
        const int x {localPos.first};
        const int y {localPos.second};

        // check adjacent terrain squares to see if there should be a border between them
        const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
        const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
        const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
        const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));

        if (north == mSelection.end()) {
            mVertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x, y + 1)]));
            mVertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x + 1, y + 1)]));
        }

        if (south == mSelection.end()) {
            mVertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x, y)]));
            mVertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x + 1, y)]));
        }

        if (east == mSelection.end()) {
            mVertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y), landData->mHeights[landIndex(x + 1, y)]));
            mVertices->push_back(osg::Vec3f(toWorldCoords(x + 1), toWorldCoords(y + 1), landData->mHeights[landIndex(x + 1, y + 1)]));
        }

        if (west == mSelection.end()) {
            mVertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y), landData->mHeights[landIndex(x, y)]));
            mVertices->push_back(osg::Vec3f(toWorldCoords(x), toWorldCoords(y + 1), landData->mHeights[landIndex(x, y + 1)]));
        }
    }

    mDrawArrays->setCount(mVertices->size());
    mDrawArrays->dirty();
}
