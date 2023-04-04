#include "loadcell.hpp"

#include <limits>
#include <list>
#include <string>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        ///< Translate 8bit/24bit code (stored in refNum.mIndex) into a proper refNum
        void adjustRefNum(RefNum& refNum, const ESMReader& reader)
        {
            unsigned int local = (refNum.mIndex & 0xff000000) >> 24;

            // If we have an index value that does not make sense, assume that it was an addition
            // by the present plugin (but a faulty one)
            if (local && local <= reader.getParentFileIndices().size())
            {
                // If the most significant 8 bits are used, then this reference already exists.
                // In this case, do not spawn a new reference, but overwrite the old one.
                refNum.mIndex &= 0x00ffffff; // delete old plugin ID
                refNum.mContentFile = reader.getParentFileIndices()[local - 1];
            }
            else
            {
                // This is an addition by the present plugin. Set the corresponding plugin index.
                refNum.mContentFile = reader.getIndex();
            }
        }
    }
}

namespace ESM
{
    const std::string Cell::sDefaultWorldspace = "sys::default";

    // Some overloaded compare operators.
    bool operator==(const MovedCellRef& ref, const RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }

    bool operator==(const CellRef& ref, const RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }

    void Cell::load(ESMReader& esm, bool& isDeleted, bool saveContext)
    {
        loadNameAndData(esm, isDeleted);
        loadCell(esm, saveContext);
    }

    const ESM::RefId& Cell::updateId()
    {
        mId = generateIdForCell(isExterior(), mName, getGridX(), getGridY());
        return mId;
    }

    ESM::RefId Cell::generateIdForCell(bool exterior, std::string_view cellName, int x, int y)
    {
        if (!exterior)
        {
            return ESM::RefId::stringRefId(cellName);
        }
        else
        {
            return ESM::RefId::esm3ExteriorCell(x, y);
        }
    }

    void Cell::loadNameAndData(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;

        blank();

        bool hasData = false;
        bool isLoaded = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mName = esm.getHString();
                    break;
                case fourCC("DATA"):
                    esm.getHTSized<12>(mData);
                    hasData = true;
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.cacheSubName();
                    isLoaded = true;
                    break;
            }
        }

        if (!hasData)
            esm.fail("Missing DATA subrecord");

        updateId();
    }

    void Cell::loadCell(ESMReader& esm, bool saveContext)
    {
        bool overriding = !mName.empty();
        bool isLoaded = false;
        mHasAmbi = false;
        while (!isLoaded && esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("INTV"):
                    int waterl;
                    esm.getHT(waterl);
                    mWater = static_cast<float>(waterl);
                    mWaterInt = true;
                    break;
                case fourCC("WHGT"):
                    float waterLevel;
                    esm.getHT(waterLevel);
                    mWaterInt = false;
                    if (!std::isfinite(waterLevel))
                    {
                        if (!overriding)
                            mWater = std::numeric_limits<float>::max();
                        Log(Debug::Warning) << "Warning: Encountered invalid water level in cell " << mName
                                            << " defined in " << esm.getContext().filename;
                    }
                    else
                        mWater = waterLevel;
                    break;
                case fourCC("AMBI"):
                    esm.getHT(mAmbi);
                    mHasAmbi = true;
                    break;
                case fourCC("RGNN"):
                    mRegion = esm.getRefId();
                    break;
                case fourCC("NAM5"):
                    esm.getHT(mMapColor);
                    break;
                case fourCC("NAM0"):
                    esm.getHT(mRefNumCounter);
                    break;
                default:
                    esm.cacheSubName();
                    isLoaded = true;
                    break;
            }
        }

        if (saveContext)
        {
            mContextList.push_back(esm.getContext());
            esm.skipRecord();
        }
    }

    void Cell::postLoad(ESMReader& esm)
    {
        // Save position of the cell references and move on
        mContextList.push_back(esm.getContext());
        esm.skipRecord();
    }

    void Cell::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mName);
        esm.writeHNT("DATA", mData, 12);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        if (mData.mFlags & Interior)
        {
            if (mWaterInt)
            {
                int water = (mWater >= 0) ? (int)(mWater + 0.5) : (int)(mWater - 0.5);
                esm.writeHNT("INTV", water);
            }
            else
            {
                esm.writeHNT("WHGT", mWater);
            }

            if (mData.mFlags & QuasiEx)
                esm.writeHNOCRefId("RGNN", mRegion);
            else
            {
                // Try to avoid saving ambient lighting information when it's unnecessary.
                // This is to fix black lighting in resaved cell records that lack this information.
                if (mHasAmbi)
                    esm.writeHNT("AMBI", mAmbi, 16);
            }
        }
        else
        {
            esm.writeHNOCRefId("RGNN", mRegion);
            if (mMapColor != 0)
                esm.writeHNT("NAM5", mMapColor);
        }
    }

    void Cell::saveTempMarker(ESMWriter& esm, int tempCount) const
    {
        if (tempCount != 0)
            esm.writeHNT("NAM0", tempCount);
    }

    void Cell::restore(ESMReader& esm, int iCtx) const
    {
        esm.restoreContext(mContextList.at(iCtx));
    }

    std::string Cell::getDescription() const
    {
        const auto& nameString = mName;
        if (mData.mFlags & Interior)
            return nameString;

        std::string cellGrid = "(" + std::to_string(mData.mX) + ", " + std::to_string(mData.mY) + ")";
        if (!mName.empty())
            return nameString + ' ' + cellGrid;
        // FIXME: should use sDefaultCellname GMST instead, but it's not available in this scope
        std::string region = !mRegion.empty() ? mRegion.getRefIdString() : "Wilderness";

        return region + ' ' + cellGrid;
    }

    bool Cell::getNextRef(ESMReader& esm, CellRef& ref, bool& isDeleted)
    {
        isDeleted = false;

        // TODO: Try and document reference numbering, I don't think this has been done anywhere else.
        if (!esm.hasMoreSubs())
            return false;

        // MVRF are FRMR are present in pairs. MVRF indicates that following FRMR describes moved CellRef.
        // This function has to skip all moved CellRefs therefore read all such pairs to ignored values.
        while (esm.isNextSub("MVRF"))
        {
            MovedCellRef movedCellRef;
            esm.getHT(movedCellRef.mRefNum.mIndex);
            esm.getHNOT(movedCellRef.mTarget, "CNDT");
            CellRef skippedCellRef;
            if (!esm.peekNextSub("FRMR"))
                return false;
            bool skippedDeleted;
            skippedCellRef.load(esm, skippedDeleted);
        }

        if (esm.peekNextSub("FRMR"))
        {
            ref.load(esm, isDeleted);

            // TODO: should count the number of temp refs and validate the number

            // Identify references belonging to a parent file and adapt the ID accordingly.
            adjustRefNum(ref.mRefNum, esm);
            return true;
        }
        return false;
    }

    bool Cell::getNextRef(
        ESMReader& esm, CellRef& cellRef, bool& deleted, MovedCellRef& movedCellRef, bool& moved, GetNextRefMode mode)
    {
        deleted = false;
        moved = false;

        if (!esm.hasMoreSubs())
            return false;

        if (esm.isNextSub("MVRF"))
        {
            moved = true;
            getNextMVRF(esm, movedCellRef);
        }

        if (!esm.peekNextSub("FRMR"))
            return false;

        if ((!moved && mode == GetNextRefMode::LoadOnlyMoved) || (moved && mode == GetNextRefMode::LoadOnlyNotMoved))
        {
            skipLoadCellRef(esm);
            return true;
        }

        cellRef.load(esm, deleted);
        adjustRefNum(cellRef.mRefNum, esm);

        return true;
    }

    bool Cell::getNextMVRF(ESMReader& esm, MovedCellRef& mref)
    {
        esm.getHT(mref.mRefNum.mIndex);
        esm.getHNOT(mref.mTarget, "CNDT");

        adjustRefNum(mref.mRefNum, esm);

        return true;
    }

    void Cell::blank()
    {
        mName = "";
        mRegion = ESM::RefId();
        mWater = 0;
        mWaterInt = false;
        mMapColor = 0;
        mRefNumCounter = 0;

        mData.mFlags = 0;
        mData.mX = 0;
        mData.mY = 0;

        mHasAmbi = true;
        mAmbi.mAmbient = 0;
        mAmbi.mSunlight = 0;
        mAmbi.mFog = 0;
        mAmbi.mFogDensity = 0;
    }

}
