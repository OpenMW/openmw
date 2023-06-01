#include "cell.hpp"

#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/esm4/loadwrld.hpp>
#include <components/misc/algorithm.hpp>

#include "../mwbase/environment.hpp"
#include "esmstore.hpp"

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
        if (isExterior())
        {
            auto& worldStore = MWBase::Environment::get().getESMStore()->get<ESM4::World>();
            const ESM4::World* cellWorld = worldStore.find(mParent);
            mWaterHeight = cellWorld->mWaterLevel;
        }
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
        , mParent(ESM::Cell::sDefaultWorldspaceId)
        , mMood{
            .mAmbiantColor = cell.mAmbi.mAmbient,
            .mDirectionalColor = cell.mAmbi.mSunlight,
            .mFogColor = cell.mAmbi.mFog,
            .mFogDensity = cell.mAmbi.mFogDensity,
        }
        ,mWaterHeight(cell.mWater)
    {
        if (isExterior())
            mWaterHeight = -1.f;
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

    ESM::ExteriorCellLocation Cell::getExteriorCellLocation() const
    {
        return { mGridPos.x(), mGridPos.y(), getWorldSpace() };
    }
}
