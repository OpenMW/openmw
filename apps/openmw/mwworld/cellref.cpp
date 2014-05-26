#include "cellref.hpp"

#include <cstdlib>

#include <components/esm/objectstate.hpp>

namespace MWWorld
{

    ESM::RefNum CellRef::getRefNum() const
    {
        return mCellRef.mRefNum;
    }

    std::string CellRef::getRefId() const
    {
        return mCellRef.mRefID;
    }

    bool CellRef::getTeleport() const
    {
        return mCellRef.mTeleport;
    }

    ESM::Position CellRef::getDoorDest() const
    {
        return mCellRef.mDoorDest;
    }

    std::string CellRef::getDestCell() const
    {
        return mCellRef.mDestCell;
    }

    float CellRef::getScale() const
    {
        return mCellRef.mScale;
    }

    void CellRef::setScale(float scale)
    {
        if (scale != mCellRef.mScale)
        {
            mChanged = true;
            mCellRef.mScale = scale;
        }
    }

    ESM::Position CellRef::getPosition() const
    {
        return mCellRef.mPos;
    }

    void CellRef::setPosition(const ESM::Position &position)
    {
        mChanged = true;
        mCellRef.mPos = position;
    }

    float CellRef::getEnchantmentCharge() const
    {
        return mCellRef.mEnchantmentCharge;
    }

    void CellRef::setEnchantmentCharge(float charge)
    {
        if (charge != mCellRef.mEnchantmentCharge)
        {
            mChanged = true;
            mCellRef.mEnchantmentCharge = charge;
        }
    }

    int CellRef::getCharge() const
    {
        return mCellRef.mCharge;
    }

    void CellRef::setCharge(int charge)
    {
        if (charge != mCellRef.mCharge)
        {
            mChanged = true;
            mCellRef.mCharge = charge;
        }
    }

    std::string CellRef::getOwner() const
    {
        return mCellRef.mOwner;
    }

    void CellRef::setOwner(const std::string &owner)
    {
        if (owner != mCellRef.mOwner)
        {
            mChanged = true;
            mCellRef.mOwner = owner;
        }
    }

    std::string CellRef::getSoul() const
    {
        return mCellRef.mSoul;
    }

    void CellRef::setSoul(const std::string &soul)
    {
        if (soul != mCellRef.mSoul)
        {
            mChanged = true;
            mCellRef.mSoul = soul;
        }
    }

    std::string CellRef::getFaction() const
    {
        return mCellRef.mFaction;
    }

    void CellRef::setFaction(const std::string &faction)
    {
        if (faction != mCellRef.mFaction)
        {
            mChanged = true;
            mCellRef.mFaction = faction;
        }
    }

    int CellRef::getLockLevel() const
    {
        return mCellRef.mLockLevel;
    }

    void CellRef::setLockLevel(int lockLevel)
    {
        if (lockLevel != mCellRef.mLockLevel)
        {
            mChanged = true;
            mCellRef.mLockLevel = lockLevel;
        }
    }

    std::string CellRef::getKey() const
    {
        return mCellRef.mKey;
    }

    std::string CellRef::getTrap() const
    {
        return mCellRef.mTrap;
    }

    void CellRef::setTrap(const std::string& trap)
    {
        if (trap != mCellRef.mTrap)
        {
            mChanged = true;
            mCellRef.mTrap = trap;
        }
    }

    int CellRef::getGoldValue() const
    {
        return mCellRef.mGoldValue;
    }

    void CellRef::setGoldValue(int value)
    {
        if (value != mCellRef.mGoldValue)
        {
            mChanged = true;
            mCellRef.mGoldValue = value;
        }
    }

    void CellRef::lock(int lockLevel)
    {
        if(lockLevel!=0)
            setLockLevel(std::abs(lockLevel)); //Changes lock to locklevel, in positive
        else
            setLockLevel(std::abs(getLockLevel())); //No locklevel given, just flip the origional one
    }

    void CellRef::unlock()
    {
        setLockLevel(-std::abs(getLockLevel())); //Makes lockLevel negative
    }

    void CellRef::writeState(ESM::ObjectState &state) const
    {
        state.mRef = mCellRef;
    }

    bool CellRef::hasChanged() const
    {
        return mChanged;
    }

}
