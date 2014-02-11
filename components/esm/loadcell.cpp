#include "loadcell.hpp"

#include <string>
#include <sstream>
#include <list>

#include <boost/concept_check.hpp>

#include <components/misc/stringops.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"
#include "cellid.hpp"

namespace
{
    ///< Translate 8bit/24bit code (stored in refNum.mIndex) into a proper refNum
    void adjustRefNum (ESM::CellRef::RefNum& refNum, ESM::ESMReader& reader)
    {
        int local = (refNum.mIndex & 0xff000000) >> 24;

        if (local)
        {
            // If the most significant 8 bits are used, then this reference already exists.
            // In this case, do not spawn a new reference, but overwrite the old one.
            refNum.mIndex &= 0x00ffffff; // delete old plugin ID
            refNum.mContentFile = reader.getGameFiles()[local-1].index;
        }
        else
        {
            // This is an addition by the present plugin. Set the corresponding plugin index.
            refNum.mContentFile = reader.getIndex();
        }
    }
}

namespace ESM
{
    unsigned int Cell::sRecordId = REC_CELL;

    // Some overloaded compare operators.
    bool operator== (const MovedCellRef& ref, const CellRef::RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }

    bool operator== (const CellRef& ref, const CellRef::RefNum& refNum)
    {
        return ref.mRefNum == refNum;
    }


void Cell::load(ESMReader &esm, bool saveContext)
{
    // Ignore this for now, it might mean we should delete the entire
    // cell?
    // TODO: treat the special case "another plugin moved this ref, but we want to delete it"!
    if (esm.isNextSub("DELE")) {
        esm.skipHSub();
    }

    esm.getHNT(mData, "DATA", 12);

    mNAM0 = 0;

    if (mData.mFlags & Interior)
    {
        // Interior cells
        if (esm.isNextSub("INTV"))
        {
            int waterl;
            esm.getHT(waterl);
            mWater = (float) waterl;
            mWaterInt = true;
            mHasWaterLevelRecord = true;
        }
        else if (esm.isNextSub("WHGT"))
        {
            esm.getHT(mWater);
            mHasWaterLevelRecord = true;
        }

        // Quasi-exterior cells have a region (which determines the
        // weather), pure interior cells have ambient lighting
        // instead.
        if (mData.mFlags & QuasiEx)
            mRegion = esm.getHNOString("RGNN");
        else if (esm.isNextSub("AMBI"))
            esm.getHT(mAmbi);
    }
    else
    {
        // Exterior cells
        mRegion = esm.getHNOString("RGNN");

        mMapColor = 0;
        esm.getHNOT(mMapColor, "NAM5");
    }
    if (esm.isNextSub("NAM0")) {
        esm.getHT(mNAM0);
    }

    if (saveContext) {
        mContextList.push_back(esm.getContext());
        esm.skipRecord();
    }
}

void Cell::preLoad(ESMReader &esm) //Can't be "load" because it conflicts with function in esmtool
{
    this->load(esm, false);
}

void Cell::postLoad(ESMReader &esm)
{
    // Save position of the cell references and move on
    mContextList.push_back(esm.getContext());
    esm.skipRecord();
}

void Cell::save(ESMWriter &esm) const
{
    esm.writeHNT("DATA", mData, 12);
    if (mData.mFlags & Interior)
    {
        if (mHasWaterLevelRecord) {
            if (mWaterInt) {
                int water =
                    (mWater >= 0) ? (int) (mWater + 0.5) : (int) (mWater - 0.5);
                esm.writeHNT("INTV", water);
            } else {
                esm.writeHNT("WHGT", mWater);
            }
        }

        if (mData.mFlags & QuasiEx)
            esm.writeHNOCString("RGNN", mRegion);
        else
            esm.writeHNT("AMBI", mAmbi, 16);
    }
    else
    {
        esm.writeHNOCString("RGNN", mRegion);
        if (mMapColor != 0)
            esm.writeHNT("NAM5", mMapColor);
    }

    if (mNAM0 != 0)
        esm.writeHNT("NAM0", mNAM0);
}

void Cell::restore(ESMReader &esm, int iCtx) const
{
    esm.restoreContext(mContextList.at (iCtx));
}

std::string Cell::getDescription() const
{
    if (mData.mFlags & Interior)
    {
        return mName;
    }
    else
    {
        std::ostringstream stream;
        stream << mData.mX << ", " << mData.mY;
        return stream.str();
    }
}

bool Cell::getNextRef(ESMReader &esm, CellRef &ref, bool& deleted)
{
    // TODO: Try and document reference numbering, I don't think this has been done anywhere else.
    if (!esm.hasMoreSubs())
        return false;

    // NOTE: We should not need this check. It is a safety check until we have checked
    // more plugins, and how they treat these moved references.
    if (esm.isNextSub("MVRF")) {
        esm.skipRecord(); // skip MVRF
        esm.skipRecord(); // skip CNDT
        // That should be it, I haven't seen any other fields yet.
    }

    ref.load (esm);

    // Identify references belonging to a parent file and adapt the ID accordingly.
    adjustRefNum (ref.mRefNum, esm);

    if (esm.isNextSub("DELE"))
    {
        esm.skipHSub();
        deleted = true;
    }
    else
        deleted = false;

    return true;
}

bool Cell::getNextMVRF(ESMReader &esm, MovedCellRef &mref)
{
    esm.getHT(mref.mRefNum.mIndex);
    esm.getHNOT(mref.mTarget, "CNDT");

    adjustRefNum (mref.mRefNum, esm);

    return true;
}

    void Cell::blank()
    {
        mName.clear();
        mRegion.clear();
        mWater = 0;
        mWaterInt = false;
        mMapColor = 0;
        mNAM0 = 0;

        mData.mFlags = 0;
        mData.mX = 0;
        mData.mY = 0;

        mAmbi.mAmbient = 0;
        mAmbi.mSunlight = 0;
        mAmbi.mFog = 0;
        mAmbi.mFogDensity = 0;
    }

    void Cell::merge(Cell *original, Cell *modified)
    {
        float waterLevel = original->mWater;
        if (modified->mHasWaterLevelRecord)
        {
            waterLevel = modified->mWater;
        }
        // else: keep original water level, instead of resetting to 0

        *original = *modified;
        original->mWater = waterLevel;
    }

    CellId Cell::getCellId() const
    {
        CellId id;

        id.mPaged = !(mData.mFlags & Interior);

        if (id.mPaged)
        {
            id.mWorldspace = "default";
            id.mIndex.mX = mData.mX;
            id.mIndex.mY = mData.mY;
        }
        else
        {
            id.mWorldspace = Misc::StringUtils::lowerCase (mName);
        }

        return id;
    }
}
