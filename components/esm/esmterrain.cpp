#include <components/misc/constants.hpp>

#include "esmterrain.hpp"

ESM::LandData::LandData(const ESM::Land& land, int loadFlags)
    : mLoadFlags(loadFlags)
    , mSize(Constants::CellSizeInUnits)
    , mLandSize(ESM::Land::LAND_SIZE)
{
    ESM::Land::LandData data;
    land.loadData(loadFlags, &data);
    mLoadFlags = data.mDataLoaded;
    std::span<const float> heights(data.mHeights);
    mHeights = std::vector(heights.begin(), heights.end());

    std::span<const std::int8_t> normals(data.mNormals);
    mNormals = std::vector(normals.begin(), normals.end());

    std::span<const std::uint8_t> colors(data.mColours);
    mColors = std::vector(colors.begin(), colors.end());

    std::span<const uint16_t> textures(data.mTextures);
    mTextures = std::vector(textures.begin(), textures.end());

    mMinHeight = data.mMinHeight;
    mMaxHeight = data.mMaxHeight;
}

ESM::LandData::LandData(const ESM4::Land& land, int /*loadFlags*/)
    : mLoadFlags(land.mDataTypes) // ESM4::Land is always fully loaded. TODO: implement lazy loading
    , mSize(Constants::ESM4CellSizeInUnits)
    , mLandSize(ESM4::Land::VERTS_PER_SIDE)
{

    mMinHeight = std::numeric_limits<float>::max();
    mMaxHeight = std::numeric_limits<float>::lowest();
    mHeights.resize(ESM4::Land::LAND_NUM_VERTS);
    mTextures.resize(ESM::Land::LAND_NUM_TEXTURES);
    std::fill(mTextures.begin(), mTextures.end(), 0);

    float row_offset = land.mHeightMap.heightOffset;
    for (int y = 0; y < mLandSize; y++)
    {
        row_offset += land.mHeightMap.gradientData[y * mLandSize];

        const float heightY = row_offset * ESM4::Land::HEIGHT_SCALE;
        mHeights[y * mLandSize] = heightY;
        mMinHeight = std::min(mMinHeight, heightY);
        mMaxHeight = std::max(mMaxHeight, heightY);

        float colOffset = row_offset;
        for (int x = 1; x < mLandSize; x++)
        {
            colOffset += land.mHeightMap.gradientData[y * mLandSize + x];
            const float heightX = colOffset * ESM4::Land::HEIGHT_SCALE;
            mMinHeight = std::min(mMinHeight, heightX);
            mMaxHeight = std::max(mMaxHeight, heightX);
            mHeights[x + y * mLandSize] = heightX;
        }
    }

    std::span<const std::int8_t> normals(land.mVertNorm);
    mNormals = std::vector(normals.begin(), normals.end());

    std::span<const std::uint8_t> colors(land.mVertColr);
    mColors = std::vector(colors.begin(), colors.end());
}
