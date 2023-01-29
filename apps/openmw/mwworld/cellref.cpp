#include "cellref.hpp"

#include <cassert>

#include <components/debug/debuglog.hpp>
#include <components/esm3/objectstate.hpp>

namespace MWWorld
{
    // makes it easier to use std visit with a variant
    template <class... Ts>
    struct RefVisit : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    RefVisit(Ts...) -> RefVisit<Ts...>;

    CellRef::CellRef(const ESM::CellRef& ref)
        : mCellRef(ESM::ReferenceVariant(ref))
    {
    }

    CellRef::CellRef(const ESM4::Reference& ref)
        : mCellRef(ESM::ReferenceVariant(ref))
    {
    }

    static const ESM::RefNum emptyRefNum = {};

    const ESM::RefNum& CellRef::getRefNum() const
    {
        return std::visit(RefVisit{
                              [&](const ESM4::Reference& /*ref*/) -> const ESM::RefNum& { return emptyRefNum; },
                              [&](const ESM::CellRef& ref) -> const ESM::RefNum& { return ref.mRefNum; },
                          },
            mCellRef.mVariant);
    }

    const ESM::RefNum& CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        return std::visit(
            RefVisit{
                [&](ESM4::Reference& /*ref*/) -> const ESM::RefNum& { return emptyRefNum; },
                [&](ESM::CellRef& ref) -> const ESM::RefNum& {
                    if (!ref.mRefNum.isSet())
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
                        ref.mRefNum = lastAssignedRefNum;
                        mChanged = true;
                    }
                    return ref.mRefNum;
                },
            },
            mCellRef.mVariant);
    }

    void CellRef::unsetRefNum()
    {
        std::visit(RefVisit{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& ref) { ref.mRefNum = emptyRefNum; },
                   },
            mCellRef.mVariant);
    }

    static const std::string emptyString = "";

    const std::string& CellRef::getDestCell() const
    {
        return mCellRef.isESM4() ? emptyString : mCellRef.getEsm3().mDestCell;
    }

    void CellRef::setScale(float scale)
    {
        if (scale != getScale())
        {
            mChanged = true;
            std::visit([scale](auto&& ref) { ref.mScale = scale; }, mCellRef.mVariant);
        }
    }

    void CellRef::setPosition(const ESM::Position& position)
    {
        mChanged = true;
        std::visit([&position](auto&& ref) { ref.mPos = position; }, mCellRef.mVariant);
    }

    float CellRef::getEnchantmentCharge() const
    {
        return std::visit(RefVisit{
                              [&](const ESM4::Reference& /*ref*/) { return 0.f; },
                              [&](const ESM::CellRef& ref) { return ref.mEnchantmentCharge; },
                          },
            mCellRef.mVariant);
    }

    float CellRef::getNormalizedEnchantmentCharge(int maxCharge) const
    {
        if (maxCharge == 0)
        {
            return 0;
        }
        else if (getEnchantmentCharge() == -1)
        {
            return 1;
        }
        else
        {
            return getEnchantmentCharge() / static_cast<float>(maxCharge);
        }
    }

    void CellRef::setEnchantmentCharge(float charge)
    {
        if (charge != getEnchantmentCharge())
        {
            mChanged = true;

            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mEnchantmentCharge = charge; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setCharge(int charge)
    {
        std::visit(RefVisit{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& ref) { ref.mChargeInt = charge; },
                   },
            mCellRef.mVariant);
    }

    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        std::visit(RefVisit{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& cellRef3) {
                           cellRef3.mChargeIntRemainder += std::abs(chargeRemainder);
                           if (cellRef3.mChargeIntRemainder > 1.0f)
                           {
                               float newChargeRemainder
                                   = (cellRef3.mChargeIntRemainder - std::floor(cellRef3.mChargeIntRemainder));
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
                       },
                   },
            mCellRef.mVariant);
    }

    void CellRef::setChargeFloat(float charge)
    {
        std::visit(RefVisit{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& ref) { ref.mChargeFloat = charge; },
                   },
            mCellRef.mVariant);
    }

    const std::string& CellRef::getGlobalVariable() const
    {

        return std::visit(RefVisit{
                              [&](const ESM4::Reference& /*ref*/) -> const std::string& { return emptyString; },
                              [&](const ESM::CellRef& ref) -> const std::string& { return ref.mGlobalVariable; },
                          },
            mCellRef.mVariant);
    }

    void CellRef::resetGlobalVariable()
    {
        if (!getGlobalVariable().empty())
        {
            mChanged = true;
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mGlobalVariable.erase(); },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setFactionRank(int factionRank)
    {
        if (factionRank != getFactionRank())
        {
            mChanged = true;
            std::visit([&](auto&& ref) { ref.mFactionRank = factionRank; }, mCellRef.mVariant);
        }
    }

    void CellRef::setOwner(const ESM::RefId& owner)
    {
        if (owner != getOwner())
        {
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mOwner = owner; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setSoul(const ESM::RefId& soul)
    {
        if (soul != getSoul())
        {
            mChanged = true;
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mSoul = soul; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setFaction(const ESM::RefId& faction)
    {
        if (faction != getFaction())
        {
            mChanged = true;
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mFaction = faction; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setLockLevel(int lockLevel)
    {
        if (lockLevel != getLockLevel())
        {
            mChanged = true;
            std::visit([&](auto&& ref) { ref.mLockLevel = lockLevel; }, mCellRef.mVariant);
        }
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
        setLockLevel(-abs(getLockLevel())); // Makes lockLevel negative
    }

    void CellRef::setTrap(const ESM::RefId& trap)
    {
        if (trap != getTrap())
        {
            mChanged = true;
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mTrap = trap; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setGoldValue(int value)
    {
        if (value != getGoldValue())
        {
            mChanged = true;
            std::visit(RefVisit{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mGoldValue = value; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::writeState(ESM::ObjectState& state) const
    {
        std::visit(RefVisit{
                       [&](const ESM4::Reference& /*ref*/) {},
                       [&](const ESM::CellRef& ref) { state.mRef = ref; },
                   },
            mCellRef.mVariant);
    }
}
