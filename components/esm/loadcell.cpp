#include "loadcell.hpp"

#include <string>
#include <sstream>
#include <list>
#include <boost/concept_check.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Cell::sRecordId = REC_CELL;

/// Some overloaded compare operators.
bool operator==(const MovedCellRef& ref, int pRefnum)
{
  return (ref.mRefnum == pRefnum);
}

bool operator==(const CellRef& ref, int pRefnum)
{
  return (ref.mRefnum == pRefnum);
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
        }
        else if (esm.isNextSub("WHGT"))
            esm.getHT(mWater);

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
        if (mWater != -1) {
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

bool Cell::getNextRef(ESMReader &esm, CellRef &ref)
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

    // NAM0 sometimes appears here, sometimes further on
    ref.mNam0 = 0;
    if (esm.isNextSub("NAM0"))
    {
        esm.getHT(ref.mNam0);
        //esm.getHNOT(NAM0, "NAM0");
    }

    esm.getHNT(ref.mRefnum, "FRMR");
    ref.mRefID = esm.getHNString("NAME");

    // Identify references belonging to a parent file and adapt the ID accordingly.
    int local = (ref.mRefnum & 0xff000000) >> 24;
    size_t global = esm.getIndex() + 1;
    if (local)
    {
        // If the most significant 8 bits are used, then this reference already exists.
        // In this case, do not spawn a new reference, but overwrite the old one.
        ref.mRefnum &= 0x00ffffff; // delete old plugin ID
        const std::vector<Header::MasterData> &masters = esm.getGameFiles();
        global = masters[local-1].index + 1;
        ref.mRefnum |= global << 24; // insert global plugin ID
    }
    else
    {
        // This is an addition by the present plugin. Set the corresponding plugin index.
        ref.mRefnum |= global << 24; // insert global plugin ID
    }

    // getHNOT will not change the existing value if the subrecord is
    // missing
    ref.mScale = 1.0;
    esm.getHNOT(ref.mScale, "XSCL");

    // TODO: support loading references from saves, there are tons of keys not recognized yet.
    // The following is just an incomplete list.
    if (esm.isNextSub("ACTN"))
        esm.skipHSub();
    if (esm.isNextSub("STPR"))
        esm.skipHSub();
    if (esm.isNextSub("ACDT"))
        esm.skipHSub();
    if (esm.isNextSub("ACSC"))
        esm.skipHSub();
    if (esm.isNextSub("ACSL"))
        esm.skipHSub();
    if (esm.isNextSub("CHRD"))
        esm.skipHSub();
    else if (esm.isNextSub("CRED")) // ???
        esm.skipHSub();

    ref.mOwner = esm.getHNOString("ANAM");
    ref.mGlob = esm.getHNOString("BNAM");
    ref.mSoul = esm.getHNOString("XSOL");

    ref.mFaction = esm.getHNOString("CNAM");
    ref.mFactIndex = -2;
    esm.getHNOT(ref.mFactIndex, "INDX");

    ref.mGoldValue = 1;
    ref.mCharge = -1;
    ref.mEnchantmentCharge = -1;

    esm.getHNOT(ref.mEnchantmentCharge, "XCHG");

    esm.getHNOT(ref.mCharge, "INTV");

    esm.getHNOT(ref.mGoldValue, "NAM9");

    // Present for doors that teleport you to another cell.
    if (esm.isNextSub("DODT"))
    {
        ref.mTeleport = true;
        esm.getHT(ref.mDoorDest);
        ref.mDestCell = esm.getHNOString("DNAM");
    } else {
        ref.mTeleport = false;
    }

    // Integer, despite the name suggesting otherwise
    ref.mLockLevel = -1;
    esm.getHNOT(ref.mLockLevel, "FLTV");
    ref.mKey = esm.getHNOString("KNAM");
    ref.mTrap = esm.getHNOString("TNAM");

    ref.mReferenceBlocked = -1;
    ref.mFltv = 0;
    esm.getHNOT(ref.mReferenceBlocked, "UNAM");
    esm.getHNOT(ref.mFltv, "FLTV");

    esm.getHNOT(ref.mPos, "DATA", 24);

    // Number of references in the cell? Maximum once in each cell,
    // but not always at the beginning, and not always right. In other
    // words, completely useless.
    // Update: Well, maybe not completely useless. This might actually be
    //  number_of_references + number_of_references_moved_here_Across_boundaries,
    //  and could be helpful for collecting these weird moved references.
    if (esm.isNextSub("NAM0"))
    {
        esm.getHT(ref.mNam0);
        //esm.getHNOT(NAM0, "NAM0");
    }

    if (esm.isNextSub("DELE")) {
        esm.skipHSub();
        ref.mDeleted = 2; // Deleted, will not respawn.
        // TODO: find out when references do respawn.
    } else
        ref.mDeleted = 0;

    return true;
}

bool Cell::getNextMVRF(ESMReader &esm, MovedCellRef &mref)
{
    esm.getHT(mref.mRefnum);
    esm.getHNOT(mref.mTarget, "CNDT");

    // Identify references belonging to a parent file and adapt the ID accordingly.
    int local = (mref.mRefnum & 0xff000000) >> 24;
    size_t global = esm.getIndex() + 1;
    mref.mRefnum &= 0x00ffffff; // delete old plugin ID
    const std::vector<Header::MasterData> &masters = esm.getGameFiles();
    global = masters[local-1].index + 1;
    mref.mRefnum |= global << 24; // insert global plugin ID

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
}
