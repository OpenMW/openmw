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

        mFlags.hasWater = (cell.mCellFlags & ESM4::CELL_HasWater) != 0;
        mFlags.isExterior = !(cell.mCellFlags & ESM4::CELL_Interior);
        mFlags.isQuasiExterior = (cell.mCellFlags & ESM4::CELL_QuasiExt) != 0;
        mFlags.noSleep = false; // No such notion in ESM4

        mCellId.mWorldspace = Misc::StringUtils::lowerCase(cell.mEditorId);
        mCellId.mWorld = ESM::RefId::sEmpty;
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

        mFlags.hasWater = (cell.mData.mFlags & ESM::Cell::HasWater) != 0;
        mFlags.isExterior = !(cell.mData.mFlags & ESM::Cell::Interior);
        mFlags.isQuasiExterior = (cell.mData.mFlags & ESM::Cell::QuasiEx) != 0;
        mFlags.noSleep = (cell.mData.mFlags & ESM::Cell::NoSleep) != 0;

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
