
#include "cellref.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::CellRef::load (ESMReader& esm, bool wideRefNum)
{
    // NAM0 sometimes appears here, sometimes further on
    mNam0 = 0;
    if (esm.isNextSub ("NAM0"))
        esm.getHT (mNam0);

    if (wideRefNum)
        esm.getHNT (mRefNum, "FRMR", 8);
    else
        esm.getHNT (mRefNum.mIndex, "FRMR");

    mRefID = esm.getHNString ("NAME");

    // Again, UNAM sometimes appears after NAME and sometimes later.
    // Or perhaps this UNAM means something different?
    mReferenceBlocked = -1;
    esm.getHNOT (mReferenceBlocked, "UNAM");

    mScale = 1.0;
    esm.getHNOT (mScale, "XSCL");

    mOwner = esm.getHNOString ("ANAM");
    mGlob = esm.getHNOString ("BNAM");
    mSoul = esm.getHNOString ("XSOL");

    mFaction = esm.getHNOString ("CNAM");
    mFactIndex = -2;
    esm.getHNOT (mFactIndex, "INDX");

    mGoldValue = 1;
    mCharge = -1;
    mEnchantmentCharge = -1;

    esm.getHNOT (mEnchantmentCharge, "XCHG");

    esm.getHNOT (mCharge, "INTV");

    esm.getHNOT (mGoldValue, "NAM9");

    // Present for doors that teleport you to another cell.
    if (esm.isNextSub ("DODT"))
    {
        mTeleport = true;
        esm.getHT (mDoorDest);
        mDestCell = esm.getHNOString ("DNAM");
    }
    else
        mTeleport = false;

    mLockLevel = 0; //Set to 0 to indicate no lock
    esm.getHNOT (mLockLevel, "FLTV");

    mKey = esm.getHNOString ("KNAM");
    mTrap = esm.getHNOString ("TNAM");

    mFltv = 0;
    esm.getHNOT (mReferenceBlocked, "UNAM");
    esm.getHNOT (mFltv, "FLTV");

    esm.getHNOT(mPos, "DATA", 24);

    // Number of references in the cell? Maximum once in each cell,
    // but not always at the beginning, and not always right. In other
    // words, completely useless.
    // Update: Well, maybe not completely useless. This might actually be
    //  number_of_references + number_of_references_moved_here_Across_boundaries,
    //  and could be helpful for collecting these weird moved references.
    if (esm.isNextSub ("NAM0"))
        esm.getHT (mNam0);
}

void ESM::CellRef::save (ESMWriter &esm, bool wideRefNum, bool inInventory) const
{
    if (wideRefNum)
        esm.writeHNT ("FRMR", mRefNum, 8);
    else
        esm.writeHNT ("FRMR", mRefNum.mIndex, 4);

    esm.writeHNCString("NAME", mRefID);

    if (mScale != 1.0) {
        esm.writeHNT("XSCL", mScale);
    }

    esm.writeHNOCString("ANAM", mOwner);
    esm.writeHNOCString("BNAM", mGlob);
    esm.writeHNOCString("XSOL", mSoul);

    esm.writeHNOCString("CNAM", mFaction);
    if (mFactIndex != -2) {
        esm.writeHNT("INDX", mFactIndex);
    }

    if (mEnchantmentCharge != -1)
        esm.writeHNT("XCHG", mEnchantmentCharge);

    if (mCharge != -1)
        esm.writeHNT("INTV", mCharge);

    if (mGoldValue != 1) {
        esm.writeHNT("NAM9", mGoldValue);
    }

    if (mTeleport && !inInventory)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (mLockLevel != 0 && !inInventory) {
            esm.writeHNT("FLTV", mLockLevel);
    }

    if (!inInventory)
        esm.writeHNOCString ("KNAM", mKey);

    if (!inInventory)
        esm.writeHNOCString ("TNAM", mTrap);

    if (mReferenceBlocked != -1)
        esm.writeHNT("UNAM", mReferenceBlocked);

    if (mFltv != 0 && !inInventory)
        esm.writeHNT("FLTV", mFltv);

    if (!inInventory)
        esm.writeHNT("DATA", mPos, 24);

    if (mNam0 != 0 && !inInventory)
        esm.writeHNT("NAM0", mNam0);
}

void ESM::CellRef::blank()
{
    mRefNum.mIndex = 0;
    mRefNum.mContentFile = -1;
    mRefID.clear();
    mScale = 1;
    mOwner.clear();
    mGlob.clear();
    mSoul.clear();
    mFaction.clear();
    mFactIndex = -1;
    mCharge = 0;
    mEnchantmentCharge = 0;
    mGoldValue = 0;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = 0;
    mFltv = 0;
    mNam0 = 0;

    for (int i=0; i<3; ++i)
    {
        mDoorDest.pos[i] = 0;
        mDoorDest.rot[i] = 0;
        mPos.pos[i] = 0;
        mPos.rot[i] = 0;
    }
}

bool ESM::operator== (const CellRef::RefNum& left, const CellRef::RefNum& right)
{
    return left.mIndex==right.mIndex && left.mContentFile==right.mContentFile;
}
