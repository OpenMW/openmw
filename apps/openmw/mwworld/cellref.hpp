#ifndef OPENMW_MWWORLD_CELLREF_H
#define OPENMW_MWWORLD_CELLREF_H

#include <string_view>

#include <components/esm/esmbridge.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm4/loadrefr.hpp>

namespace ESM
{
    struct Enchantment;
    struct ObjectState;
}

namespace MWWorld
{

    /// \brief Encapsulated variant of ESM::CellRef with change tracking
    class CellRef
    {
    protected:
    public:
        explicit CellRef(const ESM::CellRef& ref);

        explicit CellRef(const ESM4::Reference& ref);
        explicit CellRef(const ESM4::ActorCharacter& ref);

        // Note: Currently unused for items in containers
        ESM::RefNum getRefNum() const noexcept;

        // Returns RefNum.
        // If RefNum is not set, assigns a generated one and changes the "lastAssignedRefNum" counter.
        ESM::RefNum getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum);

        void setRefNum(ESM::RefNum refNum);

        // Set RefNum to its default state.
        void unsetRefNum() { setRefNum({}); }

        /// Does the RefNum have a content file?
        bool hasContentFile() const { return getRefNum().hasContentFile(); }

        // Id of object being referenced
        ESM::RefId getRefId() const
        {
            struct Visitor
            {
                ESM::RefId operator()(const ESM::CellRef& ref) { return ref.mRefID; }
                ESM::RefId operator()(const ESM4::Reference& ref) { return ref.mBaseObj; }
                ESM::RefId operator()(const ESM4::ActorCharacter& ref) { return ref.mBaseObj; }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }

        // For doors - true if this door teleports to somewhere else, false
        // if it should open through animation.
        bool getTeleport() const
        {
            struct Visitor
            {
                bool operator()(const ESM::CellRef& ref) { return ref.mTeleport; }
                bool operator()(const ESM4::Reference& ref) { return !ref.mDoor.destDoor.isZeroOrUnset(); }
                bool operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }

        // Teleport location for the door, if this is a teleporting door.
        ESM::Position getDoorDest() const;

        // Destination cell for doors (optional)
        ESM::RefId getDestCell() const;

        // Scale applied to mesh
        float getScale() const
        {
            return std::visit([&](auto&& ref) { return ref.mScale; }, mCellRef.mVariant);
        }
        void setScale(float scale);

        // The *original* position and rotation as it was given in the Construction Set.
        // Current position and rotation of the object is stored in RefData.
        const ESM::Position& getPosition() const
        {
            return std::visit([](auto&& ref) -> const ESM::Position& { return ref.mPos; }, mCellRef.mVariant);
        }
        void setPosition(const ESM::Position& position);

        // Remaining enchantment charge. This could be -1 if the charge was not touched yet (i.e. full).
        float getEnchantmentCharge() const;

        // Remaining enchantment charge rescaled to the supplied maximum charge (such as one of the enchantment).
        float getNormalizedEnchantmentCharge(const ESM::Enchantment& enchantment) const;

        void setEnchantmentCharge(float charge);

        // For weapon or armor, this is the remaining item health.
        // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
        // If this returns int(-1) it means full health.
        int getCharge() const
        {
            struct Visitor
            {
                int operator()(const ESM::CellRef& ref) { return ref.mChargeInt; }
                int operator()(const ESM4::Reference& /*ref*/) { return 0; }
                int operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        float getChargeFloat() const
        {
            struct Visitor
            {
                float operator()(const ESM::CellRef& ref) { return ref.mChargeFloat; }
                float operator()(const ESM4::Reference& /*ref*/) { return 0; }
                float operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        } // Implemented as union with int charge
        float getChargeIntRemainder() const
        {
            struct Visitor
            {
                float operator()(const ESM::CellRef& ref) { return ref.mChargeIntRemainder; }
                float operator()(const ESM4::Reference& /*ref*/) { return 0; }
                float operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setCharge(int charge);
        void setChargeFloat(float charge);
        void applyChargeRemainderToBeSubtracted(float chargeRemainder); // Stores remainders and applies if <= -1

        // Stores fractional part of mChargeInt
        void setChargeIntRemainder(float chargeRemainder);

        // The NPC that owns this object (and will get angry if you steal it)
        ESM::RefId getOwner() const
        {
            return std::visit([](auto&& ref) -> ESM::RefId { return ref.mOwner; }, mCellRef.mVariant);
        }
        void setOwner(const ESM::RefId& owner);

        // Name of a global variable. If the global variable is set to '1', using the object is temporarily allowed
        // even if it has an Owner field.
        // Used by bed rent scripts to allow the player to use the bed for the duration of the rent.
        const std::string& getGlobalVariable() const;

        void resetGlobalVariable();

        // ID of creature trapped in this soul gem
        ESM::RefId getSoul() const
        {
            struct Visitor
            {
                ESM::RefId operator()(const ESM::CellRef& ref) { return ref.mSoul; }
                ESM::RefId operator()(const ESM4::Reference& /*ref*/) { return ESM::RefId(); }
                ESM::RefId operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setSoul(const ESM::RefId& soul);

        // The faction that owns this object (and will get angry if
        // you take it and are not a faction member)
        ESM::RefId getFaction() const
        {
            struct Visitor
            {
                ESM::RefId operator()(const ESM::CellRef& ref) { return ref.mFaction; }
                ESM::RefId operator()(const ESM4::Reference& /*ref*/) { return ESM::RefId(); }
                ESM::RefId operator()(const ESM4::ActorCharacter& /*ref*/) { return ESM::RefId(); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setFaction(const ESM::RefId& faction);

        // PC faction rank required to use the item. Sometimes is -1, which means "any rank".
        void setFactionRank(int factionRank);
        int getFactionRank() const
        {
            struct Visitor
            {
                int operator()(const ESM::CellRef& ref) { return ref.mFactionRank; }
                int operator()(const ESM4::Reference& ref) { return ref.mFactionRank; }
                int operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }

        // Lock level for doors and containers
        // Positive for a locked door. 0 for a door that was never locked.
        // For an unlocked door, it is set to -(previous locklevel)
        int getLockLevel() const
        {
            struct Visitor
            {
                int operator()(const ESM::CellRef& ref) { return ref.mLockLevel; }
                int operator()(const ESM4::Reference& ref) { return ref.mLockLevel; }
                int operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setLockLevel(int lockLevel);
        void lock(int lockLevel);
        void unlock();
        bool isLocked() const;
        void setLocked(bool locked);
        // Key and trap ID names, if any
        ESM::RefId getKey() const
        {
            struct Visitor
            {
                ESM::RefId operator()(const ESM::CellRef& ref) { return ref.mKey; }
                ESM::RefId operator()(const ESM4::Reference& ref) { return ref.mKey; }
                ESM::RefId operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setKey(const ESM::RefId& key);
        ESM::RefId getTrap() const
        {
            struct Visitor
            {
                ESM::RefId operator()(const ESM::CellRef& ref) { return ref.mTrap; }
                ESM::RefId operator()(const ESM4::Reference& /*ref*/) { return ESM::RefId(); }
                ESM::RefId operator()(const ESM4::ActorCharacter&) { throw std::logic_error("Not applicable"); }
            };
            return std::visit(Visitor(), mCellRef.mVariant);
        }
        void setTrap(const ESM::RefId& trap);

        int getCount(bool absolute = true) const
        {
            struct Visitor
            {
                int operator()(const ESM::CellRef& ref) { return ref.mCount; }
                int operator()(const ESM4::Reference& ref) { return ref.mCount; }
                int operator()(const ESM4::ActorCharacter& ref) { return ref.mCount; }
            };
            int count = std::visit(Visitor(), mCellRef.mVariant);
            if (absolute)
                return std::abs(count);
            return count;
        }
        void setCount(int value);

        // Write the content of this CellRef into the given ObjectState
        void writeState(ESM::ObjectState& state) const;

        // Has this CellRef changed since it was originally loaded?
        bool hasChanged() const { return mChanged; }

    private:
        bool mChanged = false;
        ESM::ReferenceVariant mCellRef;
    };

}

#endif
