#ifndef OPENMW_MWWORLD_CELLREF_H
#define OPENMW_MWWORLD_CELLREF_H

#include <string_view>

#include <components/esm/esmbridge.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm4/loadrefr.hpp>

namespace ESM
{
    struct ObjectState;
}

namespace MWWorld
{

    /// \brief Encapsulated variant of ESM::CellRef with change tracking
    class CellRef
    {
    protected:
    public:
        CellRef(const ESM::CellRef& ref);

        CellRef(const ESM4::Reference& ref);

        // Note: Currently unused for items in containers
        const ESM::RefNum& getRefNum() const;

        // Returns RefNum.
        // If RefNum is not set, assigns a generated one and changes the "lastAssignedRefNum" counter.
        const ESM::RefNum& getOrAssignRefNum(ESM::RefNum& lastAssignedRefNum);

        // Set RefNum to its default state.
        void unsetRefNum();

        /// Does the RefNum have a content file?
        bool hasContentFile() const { return getRefNum().hasContentFile(); }

        // Id of object being referenced
        const ESM::RefId& getRefId() const
        {
            return mCellRef.isESM4() ? mCellRef.getEsm4().mBaseObj : mCellRef.getEsm3().mRefID;
        }

        // For doors - true if this door teleports to somewhere else, false
        // if it should open through animation.
        bool getTeleport() const { return mCellRef.isESM4() ? false : mCellRef.getEsm3().mTeleport; }

        // Teleport location for the door, if this is a teleporting door.
        const ESM::Position& getDoorDest() const
        {
            return mCellRef.isESM4() ? mCellRef.getEsm4().mDoor.destPos : mCellRef.getEsm3().mDoorDest;
        }

        // Destination cell for doors (optional)
        const std::string& getDestCell() const;

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
        float getNormalizedEnchantmentCharge(int maxCharge) const;

        void setEnchantmentCharge(float charge);

        // For weapon or armor, this is the remaining item health.
        // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
        // If this returns int(-1) it means full health.
        int getCharge() const { return mCellRef.isESM4() ? 0 : mCellRef.getEsm3().mChargeInt; }
        float getChargeFloat() const
        {
            return mCellRef.isESM4() ? 0.f : mCellRef.getEsm3().mChargeFloat;
        } // Implemented as union with int charge
        void setCharge(int charge);
        void setChargeFloat(float charge);
        void applyChargeRemainderToBeSubtracted(float chargeRemainder); // Stores remainders and applies if > 1

        // The NPC that owns this object (and will get angry if you steal it)
        const ESM::RefId& getOwner() const
        {
            return mCellRef.isESM4() ? ESM::RefId::sEmpty : mCellRef.getEsm3().mOwner;
        }
        void setOwner(const ESM::RefId& owner);

        // Name of a global variable. If the global variable is set to '1', using the object is temporarily allowed
        // even if it has an Owner field.
        // Used by bed rent scripts to allow the player to use the bed for the duration of the rent.
        const std::string& getGlobalVariable() const;

        void resetGlobalVariable();

        // ID of creature trapped in this soul gem
        const ESM::RefId& getSoul() const { return mCellRef.isESM4() ? ESM::RefId::sEmpty : mCellRef.getEsm3().mSoul; }
        void setSoul(const ESM::RefId& soul);

        // The faction that owns this object (and will get angry if
        // you take it and are not a faction member)
        const ESM::RefId& getFaction() const
        {
            return mCellRef.isESM4() ? ESM::RefId::sEmpty : mCellRef.getEsm3().mFaction;
        }
        void setFaction(const ESM::RefId& faction);

        // PC faction rank required to use the item. Sometimes is -1, which means "any rank".
        void setFactionRank(int factionRank);
        int getFactionRank() const
        {
            return std::visit([&](auto&& ref) { return ref.mFactionRank; }, mCellRef.mVariant);
        }

        // Lock level for doors and containers
        // Positive for a locked door. 0 for a door that was never locked.
        // For an unlocked door, it is set to -(previous locklevel)
        int getLockLevel() const
        {
            return std::visit([](auto&& ref) { return static_cast<int>(ref.mLockLevel); }, mCellRef.mVariant);
        }
        void setLockLevel(int lockLevel);
        void lock(int lockLevel);
        void unlock();
        // Key and trap ID names, if any
        const ESM::RefId& getKey() const
        {
            return std::visit([](auto&& ref) -> const ESM::RefId& { return ref.mKey; }, mCellRef.mVariant);
        }
        const ESM::RefId& getTrap() const { return mCellRef.isESM4() ? ESM::RefId::sEmpty : mCellRef.getEsm3().mTrap; }
        void setTrap(const ESM::RefId& trap);

        // This is 5 for Gold_005 references, 100 for Gold_100 and so on.
        int getGoldValue() const { return mCellRef.isESM4() ? 0 : mCellRef.getEsm3().mGoldValue; }
        void setGoldValue(int value);

        // Write the content of this CellRef into the given ObjectState
        void writeState(ESM::ObjectState& state) const;

        // Has this CellRef changed since it was originally loaded?
        bool hasChanged() const { return mChanged; }

    private:
        bool mChanged;
        ESM::ReferenceVariant mCellRef;
    };

}

#endif
