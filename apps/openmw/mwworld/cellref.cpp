#include "cellref.hpp"

#include <cassert>

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/objectstate.hpp>
#include <components/esm4/loadrefr.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/cellutils.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace MWWorld
{
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
        return std::visit(ESM::VisitOverload{
                              [&](const ESM4::Reference& ref) -> const ESM::RefNum& { return ref.mId; },
                              [&](const ESM::CellRef& ref) -> const ESM::RefNum& { return ref.mRefNum; },
                          },
            mCellRef.mVariant);
    }

    const ESM::RefNum& CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        ESM::RefNum& refNum = std::visit(ESM::VisitOverload{
                                             [&](ESM4::Reference& ref) -> ESM::RefNum& { return ref.mId; },
                                             [&](ESM::CellRef& ref) -> ESM::RefNum& { return ref.mRefNum; },
                                         },
            mCellRef.mVariant);
        if (!refNum.isSet())
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
            refNum = lastAssignedRefNum;
            mChanged = true;
        }
        return refNum;
    }

    void CellRef::unsetRefNum()
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& ref) { ref.mId = emptyRefNum; },
                       [&](ESM::CellRef& ref) { ref.mRefNum = emptyRefNum; },
                   },
            mCellRef.mVariant);
    }

    static const std::string emptyString = "";

    ESM::Position CellRef::getDoorDest() const
    {

        return std::visit(ESM::VisitOverload{
                              [&](const ESM4::Reference& ref) { return ref.mDoor.destPos; },
                              [&](const ESM::CellRef& ref) -> ESM::Position { return ref.mDoorDest; },
                          },
            mCellRef.mVariant);
    }

    ESM::RefId CellRef::getDestCell() const
    {
        auto esm3Visit = [&](const ESM::CellRef& ref) -> ESM::RefId {
            if (!ref.mDestCell.empty())
            {
                return ESM::RefId::stringRefId(ref.mDestCell);
            }
            else
            {
                const osg::Vec2i index = positionToCellIndex(ref.mDoorDest.pos[0], ref.mDoorDest.pos[1]);
                return ESM::RefId::esm3ExteriorCell(index.x(), index.y());
            }
        };
        auto esm4Visit = [&](const ESM4::Reference& ref) -> ESM::RefId {
            if (ref.mDoor.destDoor.isZeroOrUnset())
                return ESM::RefId::sEmpty;
            const ESM4::Reference* refDest
                = MWBase::Environment::get().getWorld()->getStore().get<ESM4::Reference>().searchStatic(
                    ref.mDoor.destDoor);
            if (refDest)
                return refDest->mParent;
            return ESM::RefId::sEmpty;
        };

        return std::visit(ESM::VisitOverload{ esm3Visit, esm4Visit }, mCellRef.mVariant);
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
        return std::visit(ESM::VisitOverload{
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

            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mEnchantmentCharge = charge; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setCharge(int charge)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& ref) { ref.mChargeInt = charge; },
                   },
            mCellRef.mVariant);
    }

    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        auto esm3Visit = [&](ESM::CellRef& cellRef3) {
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
        };
        std::visit(
            ESM::VisitOverload{
                [&](ESM4::Reference& /*ref*/) {},
                esm3Visit,
            },
            mCellRef.mVariant);
    }

    void CellRef::setChargeFloat(float charge)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM::CellRef& ref) { ref.mChargeFloat = charge; },
                   },
            mCellRef.mVariant);
    }

    const std::string& CellRef::getGlobalVariable() const
    {
        return std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
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
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM::CellRef& ref) { ref.mGoldValue = value; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::writeState(ESM::ObjectState& state) const
    {
        std::visit(ESM::VisitOverload{
                       [&](const ESM4::Reference& /*ref*/) {},
                       [&](const ESM::CellRef& ref) { state.mRef = ref; },
                   },
            mCellRef.mVariant);
    }
}
