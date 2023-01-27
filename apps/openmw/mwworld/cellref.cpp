#include "cellref.hpp"

#include <cassert>

#include <components/debug/debuglog.hpp>
#include <components/esm3/objectstate.hpp>

namespace MWWorld
{
    CellRef::CellRef(const ESM::CellRef& ref)
        : mCellRef(ESM::ReferenceVariant(ref))
    {
        mChanged = false;
        mSoul = ref.mSoul;
        mTrap = ref.mTrap;
        mKey = ref.mKey;
        mFaction = ref.mFaction;
        mOwner = ref.mOwner;
        mReferenceType = ref.mRefID;
        mPos = ref.mPos;
        mDoorDest = ref.mDoorDest;
        mRefNum = ref.mRefNum;
        mGlobalVariable = ref.mGlobalVariable;
        mDestCell = ref.mDestCell;

        mLockLevel = ref.mLockLevel;
        mGoldValue = ref.mGoldValue;
        mFactionRank = ref.mFactionRank;
        mEnchantmentCharge = ref.mEnchantmentCharge;

        mScale = ref.mScale;
    }

    CellRef::CellRef(const ESM4::Reference& ref)
        : mCellRef(ESM::ReferenceVariant(ref))
    {

        mChanged = false;

        mReferenceType = ref.mBaseObj;
        mPos = { { ref.mPlacement.pos.x, ref.mPlacement.pos.y, ref.mPlacement.pos.z },
            { ref.mPlacement.rot.x, ref.mPlacement.rot.y, ref.mPlacement.rot.z } };

        mRefNum = {};
        mDoorDest = {};

        mLockLevel = ref.mLockLevel;
        mFactionRank = ref.mFactionRank;
        mGoldValue = 0;
        mEnchantmentCharge = 0;

        mScale = ref.mScale;
    }

    const ESM::RefNum& CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        if (!mRefNum.isSet())
        {
            // Generated RefNums have negative mContentFile
            assert(lastAssignedRefNum.mContentFile < 0);
            lastAssignedRefNum.mIndex++;
            if (lastAssignedRefNum.mIndex == 0) // mIndex overflow, so mContentFile should be changed
            {
                if (lastAssignedRefNum.mContentFile > std::numeric_limits<int32_t>::min())
                    lastAssignedRefNum.mContentFile--;
                else
                    Log(Debug::Error) << "RefNum counter overflow in CellRef::getOrAssignRefNum";
            }
            mRefNum = lastAssignedRefNum;
            mChanged = true;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mRefNum = mRefNum;
        return mRefNum;
    }

    void CellRef::unsetRefNum()
    {
        mRefNum = ESM::RefNum{};
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mRefNum = mRefNum;
    }

    void CellRef::setScale(float scale)
    {
        if (scale != mScale)
        {
            mChanged = true;
            mScale = scale;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mScale = Scale;
    }

    void CellRef::setPosition(const ESM::Position& position)
    {
        mChanged = true;
        mPos = position;
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mPos = position;
    }

    float CellRef::getNormalizedEnchantmentCharge(int maxCharge) const
    {
        if (maxCharge == 0)
        {
            return 0;
        }
        else if (mEnchantmentCharge == -1)
        {
            return 1;
        }
        else
        {
            return mEnchantmentCharge / static_cast<float>(maxCharge);
        }
    }

    void CellRef::setEnchantmentCharge(float charge)
    {
        if (charge != mEnchantmentCharge)
        {
            mChanged = true;
            mEnchantmentCharge = charge;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mEnchantmentCharge = mEnchantmentCharge;
    }

    void CellRef::setCharge(int charge)
    {
        if (mCellRef.isESM4())
            return;

        auto& cellRef3 = mCellRef.getEsm3();
        if (charge != cellRef3.mChargeInt)
        {
            mChanged = true;
            cellRef3.mChargeInt = charge;
        }
    }

    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        if (mCellRef.isESM4())
            return;

        auto& cellRef3 = mCellRef.getEsm3();
        cellRef3.mChargeIntRemainder += std::abs(chargeRemainder);
        if (cellRef3.mChargeIntRemainder > 1.0f)
        {
            float newChargeRemainder = (cellRef3.mChargeIntRemainder - std::floor(cellRef3.mChargeIntRemainder));
            if (cellRef3.mChargeInt <= static_cast<int>(cellRef3.mChargeIntRemainder))
            {
                cellRef3.mChargeInt = 0;
            }
            else
            {
                cellRef3.mChargeInt -= static_cast<int>(cellRef3.mChargeIntRemainder);
            }
            cellRef3.mChargeIntRemainder = newChargeRemainder;
        }
    }

    void CellRef::setChargeFloat(float charge)
    {
        if (mCellRef.isESM4())
            return;

        auto& cellRef3 = mCellRef.getEsm3();
        if (charge != cellRef3.mChargeFloat)
        {
            mChanged = true;
            cellRef3.mChargeFloat = charge;
        }
    }

    void CellRef::resetGlobalVariable()
    {
        if (!mGlobalVariable.empty())
        {
            mChanged = true;
            mGlobalVariable.erase();
        }

        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mGlobalVariable = mGlobalVariable;
    }

    void CellRef::setFactionRank(int factionRank)
    {
        if (factionRank != mFactionRank)
        {
            mChanged = true;
            mFactionRank = factionRank;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mFactionRank = mFactionRank;
    }

    void CellRef::setOwner(const ESM::RefId& owner)
    {
        if (owner != mOwner)
        {
            mChanged = true;
            mOwner = owner;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mOwner = mOwner;
    }

    void CellRef::setSoul(const ESM::RefId& soul)
    {
        if (soul != mSoul)
        {
            mChanged = true;
            mSoul = soul;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mSoul = mSoul;
    }

    void CellRef::setFaction(const ESM::RefId& faction)
    {
        if (faction != mFaction)
        {
            mChanged = true;
            mFaction = faction;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mFaction = mFaction;
    }

    void CellRef::setLockLevel(int lockLevel)
    {
        if (lockLevel != mLockLevel)
        {
            mChanged = true;
            mLockLevel = lockLevel;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mLockLevel = mLockLevel;
    }

    void CellRef::lock(int lockLevel)
    {
        if (lockLevel != 0)
            setLockLevel(abs(lockLevel)); // Changes lock to locklevel, if positive
        else
            setLockLevel(ESM::UnbreakableLock); // If zero, set to max lock level
    }

    void CellRef::unlock()
    {
        setLockLevel(-abs(mLockLevel)); // Makes lockLevel negative
    }

    void CellRef::setTrap(const ESM::RefId& trap)
    {
        if (trap != mTrap)
        {
            mChanged = true;
            mTrap = trap;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mTrap = mTrap;
    }

    void CellRef::setGoldValue(int value)
    {
        if (value != mGoldValue)
        {
            mChanged = true;
            mGoldValue = value;
        }
        if (!mCellRef.isESM4())
            mCellRef.getEsm3().mGoldValue = mGoldValue;
    }

    void CellRef::writeState(ESM::ObjectState& state) const
    {
        if (!mCellRef.isESM4())
        {
            auto& cellRef3 = mCellRef.getEsm3();
            state.mRef = cellRef3;
        }
    }

}
