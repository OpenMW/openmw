#include "cell.hpp"

#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/misc/algorithm.hpp>

namespace MWWorld
{
    Cell::Cell(const ESM4::Cell& cell)
        : ESM::CellVariant(cell)
        , mIsExterior(!(cell.mCellFlags & ESM4::CELL_Interior))
        , mIsQuasiExterior(cell.mCellFlags & ESM4::CELL_QuasiExt)
        , mHasWater(cell.mCellFlags & ESM4::CELL_HasWater)
        , mNoSleep(false) // No such notion in ESM4
        , mGridPos(cell.mX, cell.mY)
        , mDisplayname(cell.mFullName)
        , mNameID(cell.mEditorId)
        , mRegion(ESM::RefId()) // Unimplemented for now
        , mId(cell.mId)
        , mParent(cell.mParent)
        ,mMood{
            .mAmbiantColor = cell.mLighting.ambient,
            .mDirectionalColor = cell.mLighting.directional,
            .mFogColor = cell.mLighting.fogColor,
            // TODO: use ESM4::Lighting fog parameters
            .mFogDensity = 1.f,}
            ,mWaterHeight(cell.mWaterHeight)
    {
    }

    Cell::Cell(const ESM::Cell& cell)
        : ESM::CellVariant(cell)
        , mIsExterior(!(cell.mData.mFlags & ESM::Cell::Interior))
        , mIsQuasiExterior(cell.mData.mFlags & ESM::Cell::QuasiEx)
        , mHasWater(cell.mData.mFlags & ESM::Cell::HasWater)
        , mNoSleep(cell.mData.mFlags & ESM::Cell::NoSleep)
        , mGridPos(cell.getGridX(), cell.getGridY())
        , mDisplayname(cell.mName)
        , mNameID(cell.mName)
        , mRegion(cell.mRegion)
        , mId(cell.mId)
        , mParent(ESM::RefId::stringRefId(ESM::Cell::sDefaultWorldspace))
        , mMood{
            .mAmbiantColor = cell.mAmbi.mAmbient,
            .mDirectionalColor = cell.mAmbi.mSunlight,
            .mFogColor = cell.mAmbi.mFog,
            .mFogDensity = cell.mAmbi.mFogDensity,
        }
        ,mWaterHeight(cell.mWater)
    {
    }

    std::string Cell::getDescription() const
    {
        return ESM::visit(ESM::VisitOverload{
                              [&](const ESM::Cell& cell) { return cell.getDescription(); },
                              [&](const ESM4::Cell& cell) { return cell.mEditorId; },
                          },
            *this);
    }
    ESM::RefId Cell::getWorldSpace() const
    {
        if (isExterior())
            return mParent;
        else
            return mId;
    }
}
