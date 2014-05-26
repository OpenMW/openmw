#ifndef OPENMW_MWWORLD_CELLREF_H
#define OPENMW_MWWORLD_CELLREF_H

#include <components/esm/cellref.hpp>

namespace ESM
{
    class ObjectState;
}

namespace MWWorld
{

    /// \brief Encapsulated variant of ESM::CellRef with change tracking
    class CellRef
    {
    public:

        CellRef (const ESM::CellRef& ref)
            : mCellRef(ref)
        {
            mChanged = false;
        }

        // Note: Currently unused for items in containers
        ESM::RefNum getRefNum() const;

        // Id of object being referenced
        std::string getRefId() const;

        // For doors - true if this door teleports to somewhere else, false
        // if it should open through animation.
        bool getTeleport() const;

        // Teleport location for the door, if this is a teleporting door.
        ESM::Position getDoorDest() const;

        // Destination cell for doors (optional)
        std::string getDestCell() const;

        // Scale applied to mesh
        float getScale() const;
        void setScale(float scale);

        // Position and rotation of this object within the cell
        ESM::Position getPosition() const;
        void setPosition (const ESM::Position& position);

        // Remaining enchantment charge. This could be -1 if the charge was not touched yet (i.e. full).
        float getEnchantmentCharge() const;

        void setEnchantmentCharge(float charge);

        // For weapon or armor, this is the remaining item health.
        // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
        int getCharge() const;
        void setCharge(int charge);

        // The NPC that owns this object (and will get angry if you steal it)
        std::string getOwner() const;
        void setOwner(const std::string& owner);

        // ID of creature trapped in this soul gem
        std::string getSoul() const;
        void setSoul(const std::string& soul);

        // The faction that owns this object (and will get angry if
        // you take it and are not a faction member)
        std::string getFaction() const;
        void setFaction (const std::string& faction);

        // Lock level for doors and containers
        // Positive for a locked door. 0 for a door that was never locked.
        // For an unlocked door, it is set to -(previous locklevel)
        int getLockLevel() const;
        void setLockLevel(int lockLevel);

        //Pass a negative here to reset to the last locked value
        void lock (int lockLevel);
        void unlock ();
        bool isLocked () const;

         // Key and trap ID names, if any
        std::string getKey() const;
        std::string getTrap() const;
        void setTrap(const std::string& trap);

        // This is 5 for Gold_005 references, 100 for Gold_100 and so on.
        int getGoldValue() const;
        void setGoldValue(int value);

        // Write the content of this CellRef into the given ObjectState
        void writeState (ESM::ObjectState& state) const;

        // Has this CellRef changed since it was originally loaded?
        bool hasChanged() const;

    private:
        bool mChanged;
        ESM::CellRef mCellRef;
    };

}

#endif
