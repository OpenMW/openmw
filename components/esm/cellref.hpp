#ifndef OPENMW_ESM_CELLREF_H
#define OPENMW_ESM_CELLREF_H

#include <string>

#include "defs.hpp"

namespace ESM
{
    class ESMWriter;

    /* Cell reference. This represents ONE object (of many) inside the
    cell. The cell references are not loaded as part of the normal
    loading process, but are rather loaded later on demand when we are
    setting up a specific cell.
    */

    class CellRef
    {
        public:

            int mRefnum;           // Reference number
            std::string mRefID;    // ID of object being referenced (must be lowercase)

            float mScale;          // Scale applied to mesh

            // The NPC that owns this object (and will get angry if you steal
            // it)
            std::string mOwner;

            // I have no idea, looks like a link to a global variable?
            std::string mGlob;

            // ID of creature trapped in this soul gem (?)
            std::string mSoul;

            // ?? CNAM has a faction name, might be for objects/beds etc
            // belonging to a faction.
            std::string mFaction;

            // INDX might be PC faction rank required to use the item? Sometimes
            // is -1, which I assume means "any rank".
            int mFactIndex;

            // For weapon or armor, this is the remaining item health.
            // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
            int mCharge;

            // Remaining enchantment charge
            float mEnchantmentCharge;

            // This is 5 for Gold_005 references, 100 for Gold_100 and so on.
            int mGoldValue;

            // For doors - true if this door teleports to somewhere else, false
            // if it should open through animation.
            bool mTeleport;

            // Teleport location for the door, if this is a teleporting door.
            Position mDoorDest;

            // Destination cell for doors (optional)
            std::string mDestCell;

            // Lock level for doors and containers
            int mLockLevel;
            std::string mKey, mTrap; // Key and trap ID names, if any

            // This corresponds to the "Reference Blocked" checkbox in the construction set,
            // which prevents editing that reference.
            // -1 is not blocked, otherwise it is blocked.
            signed char mReferenceBlocked;

            // Track deleted references. 0 - not deleted, 1 - deleted, but respawns, 2 - deleted and does not respawn.
            int mDeleted;

            // Occurs in Tribunal.esm, eg. in the cell "Mournhold, Plaza
            // Brindisi Dorom", where it has the value 100. Also only for
            // activators.
            int mFltv;
            int mNam0;

            // Position and rotation of this object within the cell
            Position mPos;

            void save(ESMWriter &esm) const;

            void blank();
    };
}

#endif
