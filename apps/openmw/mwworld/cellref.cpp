#include "cellref.hpp"

#include <assert.h>

#include <components/debug/debuglog.hpp>
#include <components/esm/objectstate.hpp>

namespace MWWorld
{

    const ESM::RefNum& CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        if (!mCellRef.mRefNum.isSet())
        {
            // Generated RefNums have negative mContentFile
            assert(lastAssignedRefNum.mContentFile < 0);
            lastAssignedRefNum.mIndex++;
            if (lastAssignedRefNum.mIndex == 0)  // mIndex overflow, so mContentFile should be changed
            {
                if (lastAssignedRefNum.mContentFile > std::numeric_limits<int32_t>::min())
                    lastAssignedRefNum.mContentFile--;
                else
                    Log(Debug::Error) << "RefNum counter overflow in CellRef::getOrAssignRefNum";
            }
            mCellRef.mRefNum = lastAssignedRefNum;
            mChanged = true;
        }
        return mCellRef.mRefNum;
    }

    void CellRef::unsetRefNum()
    {
        mCellRef.mRefNum.unset();
    }

    void CellRef::setScale(float scale)
    {
        if (scale != mCellRef.mScale)
        {
            mChanged = true;
            mCellRef.mScale = scale;
        }
    }

    void CellRef::setPosition(const ESM::Position &position)
    {
        mChanged = true;
        mCellRef.mPos = position;
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

    void CellRef::setChargeFloat(float charge)
    {
        if (charge != mCellRef.mChargeFloat)
        {
            mChanged = true;
            mCellRef.mChargeFloat = charge;
        }
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

    void CellRef::setOwner(const std::string &owner)
    {
        if (owner != mCellRef.mOwner)
        {
            mChanged = true;
            mCellRef.mOwner = owner;
        }
    }

    void CellRef::setSoul(const std::string &soul)
    {
        if (soul != mCellRef.mSoul)
        {
            mChanged = true;
            mCellRef.mSoul = soul;
        }
    }

    void CellRef::setFaction(const std::string &faction)
    {
        if (faction != mCellRef.mFaction)
        {
            mChanged = true;
            mCellRef.mFaction = faction;
        }
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

    void CellRef::setTrap(const std::string& trap)
    {
        if (trap != mCellRef.mTrap)
        {
            mChanged = true;
            mCellRef.mTrap = trap;
        }
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

}
