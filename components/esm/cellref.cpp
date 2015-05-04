
#include "cellref.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::RefNum::load (ESMReader& esm, bool wide)
{
    if (wide)
        esm.getHNT (*this, "FRMR", 8);
    else
        esm.getHNT (mIndex, "FRMR");
}

void ESM::RefNum::save (ESMWriter &esm, bool wide, const std::string& tag) const
{
    if (wide)
        esm.writeHNT (tag, *this, 8);
    else
    {
        int refNum = (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff)<<24);

        esm.writeHNT (tag, refNum, 4);
    }
}


void ESM::CellRef::load (ESMReader& esm, bool wideRefNum)
{
    loadId(esm, wideRefNum);
    loadData(esm);
}

void ESM::CellRef::loadId (ESMReader& esm, bool wideRefNum)
{
    // According to Hrnchamd, this does not belong to the actual ref. Instead, it is a marker indicating that
    // the following refs are part of a "temp refs" section. A temp ref is not being tracked by the moved references system.
    // Its only purpose is a performance optimization for "immovable" things. We don't need this, and it's problematic anyway,
    // because any item can theoretically be moved by a script.
    if (esm.isNextSub ("NAM0"))
        esm.skipHSub();

    mRefNum.load (esm, wideRefNum);

    mRefID = esm.getHNString ("NAME");
}

void ESM::CellRef::loadData(ESMReader &esm)
{
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
    mChargeInt = -1;
    mEnchantmentCharge = -1;

    esm.getHNOT (mEnchantmentCharge, "XCHG");

    esm.getHNOT (mChargeInt, "INTV");

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
    mRefNum.save (esm, wideRefNum);

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

    if (mChargeInt != -1)
        esm.writeHNT("INTV", mChargeInt);

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
    mRefNum.unset();
    mRefID.clear();
    mScale = 1;
    mOwner.clear();
    mGlobalVariable.clear();
    mSoul.clear();
    mFaction.clear();
    mFactionRank = -2;
    mChargeInt = -1;
    mEnchantmentCharge = -1;
    mGoldValue = 0;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = -1;
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
