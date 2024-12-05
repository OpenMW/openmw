#include "cellref.hpp"

#include <cassert>

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/objectstate.hpp>
#include <components/esm4/loadrefr.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwmechanics/spellutil.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"

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

    CellRef::CellRef(const ESM4::ActorCharacter& ref)
        : mCellRef(ESM::ReferenceVariant(ref))
    {
    }

    ESM::RefNum CellRef::getRefNum() const noexcept
    {
        return std::visit(ESM::VisitOverload{
                              [&](const ESM4::Reference& ref) -> ESM::RefNum { return ref.mId; },
                              [&](const ESM4::ActorCharacter& ref) -> ESM::RefNum { return ref.mId; },
                              [&](const ESM::CellRef& ref) -> ESM::RefNum { return ref.mRefNum; },
                          },
            mCellRef.mVariant);
    }

    ESM::RefNum CellRef::getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum)
    {
        ESM::RefNum& refNum = std::visit(ESM::VisitOverload{
                                             [&](ESM4::Reference& ref) -> ESM::RefNum& { return ref.mId; },
                                             [&](ESM4::ActorCharacter& ref) -> ESM::RefNum& { return ref.mId; },
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

    void CellRef::setRefNum(ESM::RefNum refNum)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& ref) { ref.mId = refNum; },
                       [&](ESM4::ActorCharacter& ref) { ref.mId = refNum; },
                       [&](ESM::CellRef& ref) { ref.mRefNum = refNum; },
                   },
            mCellRef.mVariant);
    }

    static const std::string emptyString = "";

    ESM::Position CellRef::getDoorDest() const
    {

        return std::visit(
            ESM::VisitOverload{
                [&](const ESM4::Reference& ref) { return ref.mDoor.destPos; },
                [&](const ESM::CellRef& ref) -> ESM::Position { return ref.mDoorDest; },
                [&](const ESM4::ActorCharacter&) -> ESM::Position { throw std::logic_error("Not applicable"); },
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
                const auto cellPos = ESM::positionToExteriorCellLocation(ref.mDoorDest.pos[0], ref.mDoorDest.pos[1]);
                return ESM::RefId::esm3ExteriorCell(cellPos.mX, cellPos.mY);
            }
        };
        auto esm4Visit = [&](const ESM4::Reference& ref) -> ESM::RefId {
            if (ref.mDoor.destDoor.isZeroOrUnset())
                return ESM::RefId();
            const ESM4::Reference* refDest
                = MWBase::Environment::get().getESMStore()->get<ESM4::Reference>().searchStatic(ref.mDoor.destDoor);
            if (refDest)
                return refDest->mParent;
            return ESM::RefId();
        };
        auto actorDestCell
            = [&](const ESM4::ActorCharacter&) -> ESM::RefId { throw std::logic_error("Not applicable"); };

        return std::visit(ESM::VisitOverload{ esm3Visit, esm4Visit, actorDestCell }, mCellRef.mVariant);
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
                              [&](const ESM4::ActorCharacter&) -> float { throw std::logic_error("Not applicable"); },
                          },
            mCellRef.mVariant);
    }

    float CellRef::getNormalizedEnchantmentCharge(const ESM::Enchantment& enchantment) const
    {
        const int maxCharge = MWMechanics::getEnchantmentCharge(enchantment);
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
                           [&](ESM4::ActorCharacter&) {},
                           [&](ESM::CellRef& ref) { ref.mEnchantmentCharge = charge; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setCharge(int charge)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM4::ActorCharacter&) {},
                       [&](ESM::CellRef& ref) { ref.mChargeInt = charge; },
                   },
            mCellRef.mVariant);
    }

    void CellRef::applyChargeRemainderToBeSubtracted(float chargeRemainder)
    {
        auto esm3Visit = [&](ESM::CellRef& cellRef3) {
            cellRef3.mChargeIntRemainder -= std::abs(chargeRemainder);
            if (cellRef3.mChargeIntRemainder <= -1.0f)
            {
                float newChargeRemainder = std::modf(cellRef3.mChargeIntRemainder, &cellRef3.mChargeIntRemainder);
                cellRef3.mChargeInt += static_cast<int>(cellRef3.mChargeIntRemainder);
                cellRef3.mChargeIntRemainder = newChargeRemainder;
                if (cellRef3.mChargeInt < 0)
                    cellRef3.mChargeInt = 0;
            }
        };
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM4::ActorCharacter&) {},
                       esm3Visit,
                   },
            mCellRef.mVariant);
    }

    void CellRef::setChargeIntRemainder(float chargeRemainder)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM4::ActorCharacter&) {},
                       [&](ESM::CellRef& ref) { ref.mChargeIntRemainder = chargeRemainder; },
                   },
            mCellRef.mVariant);
    }

    void CellRef::setChargeFloat(float charge)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& /*ref*/) {},
                       [&](ESM4::ActorCharacter&) {},
                       [&](ESM::CellRef& ref) { ref.mChargeFloat = charge; },
                   },
            mCellRef.mVariant);
    }

    const std::string& CellRef::getGlobalVariable() const
    {
        return std::visit(ESM::VisitOverload{
                              [&](const ESM4::Reference& /*ref*/) -> const std::string& { return emptyString; },
                              [&](const ESM4::ActorCharacter& /*ref*/) -> const std::string& { return emptyString; },
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
                           [&](ESM4::ActorCharacter& /*ref*/) {},
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
            std::visit(ESM::VisitOverload{
                           [&](ESM4::ActorCharacter&) {}, [&](auto&& ref) { ref.mFactionRank = factionRank; } },
                mCellRef.mVariant);
        }
    }

    void CellRef::setOwner(const ESM::RefId& owner)
    {
        if (owner != getOwner())
        {
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM4::ActorCharacter&) {},
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
                           [&](ESM4::ActorCharacter&) {},
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
                           [&](ESM4::ActorCharacter&) {},
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
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& ref) { ref.mLockLevel = lockLevel; },
                           [&](ESM4::ActorCharacter&) {},
                           [&](ESM::CellRef& ref) { ref.mLockLevel = lockLevel; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::lock(int lockLevel)
    {
        setLockLevel(lockLevel);
        setLocked(true);
    }

    void CellRef::unlock()
    {
        setLockLevel(-getLockLevel());
        setLocked(false);
    }

    bool CellRef::isLocked() const
    {
        struct Visitor
        {
            bool operator()(const ESM::CellRef& ref) { return ref.mIsLocked; }
            bool operator()(const ESM4::Reference& ref) { return ref.mIsLocked; }
            bool operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
        };
        return std::visit(Visitor(), mCellRef.mVariant);
    }

    void CellRef::setLocked(bool locked)
    {
        std::visit(ESM::VisitOverload{
                       [&](ESM4::Reference& ref) { ref.mIsLocked = locked; },
                       [&](ESM4::ActorCharacter&) {},
                       [&](ESM::CellRef& ref) { ref.mIsLocked = locked; },
                   },
            mCellRef.mVariant);
    }

    void CellRef::setTrap(const ESM::RefId& trap)
    {
        if (trap != getTrap())
        {
            mChanged = true;
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM4::ActorCharacter&) {},
                           [&](ESM::CellRef& ref) { ref.mTrap = trap; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setKey(const ESM::RefId& key)
    {
        if (key != getKey())
        {
            mChanged = true;
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& /*ref*/) {},
                           [&](ESM4::ActorCharacter&) {},
                           [&](ESM::CellRef& ref) { ref.mKey = key; },
                       },
                mCellRef.mVariant);
        }
    }

    void CellRef::setCount(int value)
    {
        if (value != getCount(false))
        {
            mChanged = true;
            std::visit(ESM::VisitOverload{
                           [&](ESM4::Reference& ref) { ref.mCount = value; },
                           [&](ESM4::ActorCharacter& ref) { ref.mCount = value; },
                           [&](ESM::CellRef& ref) { ref.mCount = value; },
                       },
                mCellRef.mVariant);
            if (value == 0)
                MWBase::Environment::get().getWorld()->removeRefScript(this);
        }
    }

    void CellRef::writeState(ESM::ObjectState& state) const
    {
        std::visit(ESM::VisitOverload{
                       [&](const ESM4::Reference& /*ref*/) {},
                       [&](const ESM4::ActorCharacter&) {},
                       [&](const ESM::CellRef& ref) { state.mRef = ref; },
                   },
            mCellRef.mVariant);
    }
}
