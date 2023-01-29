#include "cell.hpp"

#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/misc/algorithm.hpp>

namespace MWWorld
{
    Cell::Cell(const ESM4::Cell& cell)
        : ESM::CellVariant(cell)
    {

        mNameID = cell.mEditorId;
        mDisplayname = cell.mFullName;
        mGridPos.x() = cell.mX;
        mGridPos.y() = cell.mY;

        mRegion = ESM::RefId::sEmpty; // Unimplemented for now

        mFlags.mHasWater = cell.mCellFlags & ESM4::CELL_HasWater;
        mFlags.mIsExterior = !(cell.mCellFlags & ESM4::CELL_Interior);
        mFlags.mIsQuasiExterior = cell.mCellFlags & ESM4::CELL_QuasiExt;
        mFlags.mNoSleep = false; // No such notion in ESM4

        mCellId.mWorldspace = Misc::StringUtils::lowerCase(cell.mEditorId);
        mCellId.mIndex.mX = cell.getGridX();
        mCellId.mIndex.mX = cell.getGridY();
        mCellId.mPaged = isExterior();

        mMood.mAmbiantColor = cell.mLighting.ambient;
        mMood.mFogColor = cell.mLighting.fogColor;
        mMood.mDirectionalColor = cell.mLighting.directional;
        mMood.mFogDensity = cell.mLighting.fogPower;
    }

    Cell::Cell(const ESM::Cell& cell)
        : ESM::CellVariant(cell)
    {
        mNameID = cell.mName;
        mDisplayname = cell.mName;
        mGridPos.x() = cell.getGridX();
        mGridPos.y() = cell.getGridY();

        mRegion = ESM::RefId::sEmpty; // Unimplemented for now

        mFlags.mHasWater = cell.mData.mFlags & ESM::Cell::HasWater;
        mFlags.mIsExterior = !(cell.mData.mFlags & ESM::Cell::Interior);
        mFlags.mIsQuasiExterior = cell.mData.mFlags & ESM::Cell::QuasiEx;
        mFlags.mNoSleep = cell.mData.mFlags & ESM::Cell::NoSleep;

        mCellId = cell.getCellId();

        mMood.mAmbiantColor = cell.mAmbi.mAmbient;
        mMood.mFogColor = cell.mAmbi.mFog;
        mMood.mDirectionalColor = cell.mAmbi.mSunlight;
        mMood.mFogDensity = cell.mAmbi.mFogDensity;
    }

    std::string Cell::getDescription() const
    {
        return isEsm4() ? mNameID : getEsm3().getDescription();
    }
}
