
#include "cellref.hpp"

#include "esmwriter.hpp"

void ESM::CellRef::save(ESMWriter &esm) const
{
    esm.writeHNT("FRMR", mRefnum);
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

    if (mTeleport)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (mLockLevel != -1) {
        esm.writeHNT("FLTV", mLockLevel);
    }
    esm.writeHNOCString("KNAM", mKey);
    esm.writeHNOCString("TNAM", mTrap);

    if (mReferenceBlocked != -1) {
        esm.writeHNT("UNAM", mReferenceBlocked);
    }
    if (mFltv != 0) {
        esm.writeHNT("FLTV", mFltv);
    }

    esm.writeHNT("DATA", mPos, 24);
    if (mNam0 != 0) {
        esm.writeHNT("NAM0", mNam0);
    }
}

void ESM::CellRef::blank()
{
    mRefnum = 0;
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