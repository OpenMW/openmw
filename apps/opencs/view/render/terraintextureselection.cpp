#include "terraintextureselection.hpp"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <algorithm>

#include <components/esm/loadland.hpp>

CSVRender::TerrainTextureSelection::TerrainTextureSelection(osg::Group* parentNode, const CSMWorld::CellCoordinates& coords, const ESM::Land& esmLand, const ESM::Land::LandData* esmLandRight, const ESM::Land::LandData* esmLandUp, const ESM::Land::LandData* esmLandUpRight)
    :TerrainSelection (coords, esmLand, parentNode),
    mEsmLandRightCell(esmLandRight),
    mEsmLandUpCell(esmLandUp),
    mEsmLandUpRightCell(esmLandUpRight)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // Color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array{};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    geometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    // Primitive
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays{osg::PrimitiveSet::Mode::LINES};

    geometry->addPrimitiveSet(drawArrays);
    geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    mGeode = new osg::Geode();
    mGeode->addDrawable(geometry);
    getBaseNode()->addChild(mGeode);
}

void CSVRender::TerrainTextureSelection::addToSelection(osg::Vec3d worldPos)
{
    const std::pair<int, int> localPos {toTextureCoords(worldPos)};

    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
        mSelection.push_back(localPos);
}

void CSVRender::TerrainTextureSelection::toggleSelection(osg::Vec3d worldPos)
{
    const std::pair<int, int> localPos {toTextureCoords(worldPos)};
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
    osg::ref_ptr<osg::Geometry> newGeometry = new osg::Geometry();

    // Color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array{};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    newGeometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    newGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    const osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array);

    if (!mSelection.empty())
    {

        for (std::pair<int, int> localPos : mSelection) {
            int x {localPos.first};
            int y {localPos.second};

            // check adjacent terrain squares to see if there should be a border between them
            const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
            const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
            const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
            const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));
            const int textureSizeToLandSizeModifier = 4;

            // Nudge selection by 1/4th of a texture size, similar how blendmaps are nudged
            const int nudgeOffset = (ESM::Land::REAL_SIZE / ESM::Land::LAND_TEXTURE_SIZE)/4;
            const int landHeightsNudge = (ESM::Land::REAL_SIZE / ESM::Land::LAND_SIZE)/64; // Does this work with all land size configurations?

            int x1 = x * textureSizeToLandSizeModifier+landHeightsNudge;
            int x2 = x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier + landHeightsNudge;
            int y1 = y * textureSizeToLandSizeModifier - landHeightsNudge;
            int y2 = y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier - landHeightsNudge;

            int landHeightX1Y1 = calculateLandHeight(x1, y1),
                landHeightX1Y2 = calculateLandHeight(x1, y2),
                landHeightX2Y1 = calculateLandHeight(x2, y1),
                landHeightX2Y2 = calculateLandHeight(x2, y2);

            if (north == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX1Y2+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX2Y2+5));
            }

            if (south == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX1Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX2Y1+5));
            }

            if (east == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX2Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX2Y2+5));
            }

            if (west == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX1Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX1Y2+5));
            }
        }
    }

    newGeometry->setVertexArray(vertices);

    const auto drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

    drawArrays->setCount(vertices->size());

    newGeometry->addPrimitiveSet(drawArrays);

    mGeode->setDrawable(0, newGeometry);
}

int CSVRender::TerrainTextureSelection::calculateLandHeight(int x, int y)
{
    const ESM::Land::LandData* landData (getLandData());

    if (!landData)
        return 250;

    enum cellTargetType {thisCell, rightCell, upCell, upRightCell};
    cellTargetType cellTarget = thisCell;

    if (x > ESM::Land::LAND_SIZE - 1 && y < 0)
    {
        cellTarget = upRightCell;
        x = x - ESM::Land::LAND_SIZE + 1;
        y = y + ESM::Land::LAND_SIZE - 1;
    }
    if (x > ESM::Land::LAND_SIZE - 1)
    {
        cellTarget = rightCell;
        x = x - ESM::Land::LAND_SIZE + 1;
    }
    if (y < 0)
    {
        cellTarget = upCell;
        y = y + ESM::Land::LAND_SIZE - 1;
    }

    switch(cellTarget)
    {
        case thisCell : return landData->mHeights[landIndex(x,y)];
        case rightCell : return mEsmLandRightCell->mHeights[landIndex(x,y)]; // TO-DO: Get height from neighbouring cell
        case upCell: return mEsmLandUpCell->mHeights[landIndex(x,y)]; // TO-DO: Get height from neighbouring cell
        case upRightCell: return mEsmLandUpRightCell->mHeights[landIndex(x,y)];; // TO-DO: Get height from neighbouring cell
    }
    return 0;
}
