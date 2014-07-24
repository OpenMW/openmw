
#include "cellref.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::CellRef::load (ESMReader& esm, bool wideRefNum)
{
    // According to Hrnchamd, this does not belong to the actual ref. Instead, it is a marker indicating that
    // the following refs are part of a "temp refs" section. A temp ref is not being tracked by the moved references system.
    // Its only purpose is a performance optimization for "immovable" things. We don't need this, and it's problematic anyway,
    // because any item can theoretically be moved by a script.
    if (esm.isNextSub ("NAM0"))
        esm.skipHSub();

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
    mGlobalVariable = esm.getHNOString ("BNAM");
    mSoul = esm.getHNOString ("XSOL");

    mFaction = esm.getHNOString ("CNAM");
    mFactionRank = -2;
    esm.getHNOT (mFactionRank, "INDX");

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

    esm.getHNOT (mReferenceBlocked, "UNAM");
    if (esm.isNextSub("FLTV")) // no longer used
        esm.skipHSub();

    esm.getHNOT(mPos, "DATA", 24);

    if (esm.isNextSub("NAM0"))
        esm.skipHSub();
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
    esm.writeHNOCString("BNAM", mGlobalVariable);
    esm.writeHNOCString("XSOL", mSoul);

    esm.writeHNOCString("CNAM", mFaction);
    if (mFactionRank != -2) {
        esm.writeHNT("INDX", mFactionRank);
    }

    if (mEnchantmentCharge != -1)
        esm.writeHNT("XCHG", mEnchantmentCharge);

    if (mCharge != -1)
        esm.writeHNT("INTV", mCharge);

    if (mGoldValue != 1) {
        esm.writeHNT("NAM9", mGoldValue);
    }

    if (!inInventory && mTeleport)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (!inInventory && mLockLevel != 0) {
            esm.writeHNT("FLTV", mLockLevel);
    }

    if (!inInventory)
        esm.writeHNOCString ("KNAM", mKey);

    if (!inInventory)
        esm.writeHNOCString ("TNAM", mTrap);

    if (mReferenceBlocked != -1)
        esm.writeHNT("UNAM", mReferenceBlocked);

    if (!inInventory)
        esm.writeHNT("DATA", mPos, 24);
}

void ESM::CellRef::blank()
{
    mRefNum.mIndex = 0;
    mRefNum.mContentFile = -1;
    mRefID.clear();
    mScale = 1;
    mOwner.clear();
    mGlobalVariable.clear();
    mSoul.clear();
    mFaction.clear();
    mFactionRank = -2;
    mCharge = 0;
    mEnchantmentCharge = 0;
    mGoldValue = 0;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = 0;
    mTeleport = false;

    for (int i=0; i<3; ++i)
    {
        mDoorDest.pos[i] = 0;
        mDoorDest.rot[i] = 0;
        mPos.pos[i] = 0;
        mPos.rot[i] = 0;
    }
}

bool ESM::operator== (const RefNum& left, const RefNum& right)
{
    return left.mIndex==right.mIndex && left.mContentFile==right.mContentFile;
}

bool ESM::operator< (const RefNum& left, const RefNum& right)
{
    if (left.mIndex<right.mIndex)
        return true;

    if (left.mIndex>right.mIndex)
        return false;

    return left.mContentFile<right.mContentFile;
}
