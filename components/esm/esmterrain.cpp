#include "esmterrain.hpp"

#include <components/esm3/loadland.hpp>
#include <components/esm4/loadland.hpp>
#include <components/misc/constants.hpp>

namespace
{
    constexpr std::uint16_t textures[ESM::LandRecordData::sLandNumTextures]{ 0 };

    std::unique_ptr<const ESM::LandRecordData> loadData(const ESM::Land& land, int loadFlags)
    {
        std::unique_ptr<ESM::LandRecordData> result = std::make_unique<ESM::LandRecordData>();
        land.loadData(loadFlags, *result);
        return result;
    }
}

namespace ESM
{
    LandData::LandData() = default;
}

ESM::LandData::LandData(const ESM::Land& land, int loadFlags)
    : mData(loadData(land, loadFlags))
    , mLoadFlags(mData->mDataLoaded)
    , mMinHeight(mData->mMinHeight)
    , mMaxHeight(mData->mMaxHeight)
    , mSize(Constants::CellSizeInUnits)
    , mLandSize(ESM::Land::LAND_SIZE)
    , mPlugin(land.getPlugin())
    , mHeights(mData->mHeights)
    , mNormals(mData->mNormals)
    , mColors(mData->mColours)
    , mTextures(mData->mTextures)
    , mIsEsm4(false)
{
}

ESM::LandData::LandData(const ESM4::Land& land, int /*loadFlags*/)
    : mLoadFlags(land.mDataTypes) // ESM4::Land is always fully loaded. TODO: implement lazy loading
    , mHeightsData(ESM4::Land::sLandNumVerts)
    , mMinHeight(std::numeric_limits<float>::max())
    , mMaxHeight(std::numeric_limits<float>::lowest())
    , mSize(Constants::ESM4CellSizeInUnits)
    , mLandSize(ESM4::Land::sVertsPerSide)
    , mPlugin(land.mId.mContentFile)
    , mNormals(land.mVertNorm)
    , mColors(land.mVertColr)
    , mTextures(textures)
    , mIsEsm4(true)
{
    float rowOffset = land.mHeightMap.heightOffset;
    for (int y = 0; y < mLandSize; y++)
    {
        rowOffset += land.mHeightMap.gradientData[y * mLandSize];

        const float heightY = rowOffset * ESM4::Land::sHeightScale;
        mHeightsData[y * mLandSize] = heightY;
        mMinHeight = std::min(mMinHeight, heightY);
        mMaxHeight = std::max(mMaxHeight, heightY);

        float colOffset = rowOffset;
        for (int x = 1; x < mLandSize; x++)
        {
            colOffset += land.mHeightMap.gradientData[y * mLandSize + x];
            const float heightX = colOffset * ESM4::Land::sHeightScale;
            mMinHeight = std::min(mMinHeight, heightX);
            mMaxHeight = std::max(mMaxHeight, heightX);
            mHeightsData[x + y * mLandSize] = heightX;
        }
    }

    mHeights = mHeightsData;

    for (int i = 0; i < 4; ++i)
        mEsm4Textures[i] = land.mTextures[i];
}

namespace ESM
{
    LandData::~LandData() = default;
}
