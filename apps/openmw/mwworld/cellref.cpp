#include "cellref.hpp"

#include <components/esm/objectstate.hpp>

namespace MWWorld
{

    const ESM::RefNum& CellRef::getRefNum() const
    {
        return mCellRef.mRefNum;
    }

    bool CellRef::hasContentFile() const
    {
        return mCellRef.mRefNum.hasContentFile();
    }

    void CellRef::unsetRefNum()
    {
        mCellRef.mRefNum.unset();
    }

    std::string CellRef::getRefId() const
    {
        return mCellRef.mRefID;
    }

    const std::string* CellRef::getRefIdPtr() const
    {
        return &mCellRef.mRefID;
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

    float CellRef::getNormalizedEnchantmentCharge(int maxCharge) const
    {
        if (maxCharge == 0)
        {
            return 0;
        }
        else if (mCellRef.mEnchantmentCharge == -1)
        {
            return 1;
        }
        else
        {
            return mCellRef.mEnchantmentCharge / static_cast<float>(maxCharge);
        }
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
        return mCellRef.mChargeInt;
    }

    void CellRef::setCharge(int charge)
    {
        if (charge != mCellRef.mChargeInt)
        {
            mChanged = true;
            mCellRef.mChargeInt = charge;
        }
    }

    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        mCellRef.mChargeIntRemainder += std::abs(chargeRemainder);
        if (mCellRef.mChargeIntRemainder > 1.0f)
        {
            float newChargeRemainder = (mCellRef.mChargeIntRemainder - std::floor(mCellRef.mChargeIntRemainder));
            if (mCellRef.mChargeInt <= static_cast<int>(mCellRef.mChargeIntRemainder))
            {
                mCellRef.mChargeInt = 0;
            }
            else
            {
                mCellRef.mChargeInt -= static_cast<int>(mCellRef.mChargeIntRemainder);
            }
            mCellRef.mChargeIntRemainder = newChargeRemainder;
        }
    }

    float CellRef::getChargeFloat() const
    {
        return mCellRef.mChargeFloat;
    }

    void CellRef::setChargeFloat(float charge)
    {
        if (charge != mCellRef.mChargeFloat)
        {
            mChanged = true;
            mCellRef.mChargeFloat = charge;
        }
    }

    std::string CellRef::getOwner() const
    {
        return mCellRef.mOwner;
    }

    std::string CellRef::getGlobalVariable() const
    {
        return mCellRef.mGlobalVariable;
    }

    void CellRef::resetGlobalVariable()
    {
        if (!mCellRef.mGlobalVariable.empty())
        {
            mChanged = true;
            mCellRef.mGlobalVariable.erase();
        }
    }

    void CellRef::setFactionRank(int factionRank)
    {
        if (factionRank != mCellRef.mFactionRank)
        {
            mChanged = true;
            mCellRef.mFactionRank = factionRank;
        }
    }

    int CellRef::getFactionRank() const
    {
        return mCellRef.mFactionRank;
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

    void CellRef::lock(int lockLevel)
    {
        if(lockLevel != 0)
            setLockLevel(abs(lockLevel)); //Changes lock to locklevel, if positive
        else
            setLockLevel(ESM::UnbreakableLock); // If zero, set to max lock level
    }

    void CellRef::unlock()
    {
        setLockLevel(-abs(mCellRef.mLockLevel)); //Makes lockLevel negative
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

    void CellRef::writeState(ESM::ObjectState &state) const
    {
        state.mRef = mCellRef;
    }

    bool CellRef::hasChanged() const
    {
        return mChanged;
    }

}
