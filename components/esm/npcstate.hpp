#ifndef OPENMW_ESM_NPCSTATE_H
#define OPENMW_ESM_NPCSTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"

namespace ESM
{
    // format 0, saved games only

    struct NpcState : public ObjectState
    {
        InventoryState mInventory;
        NpcStats mNpcStats;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank();

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;

        virtual NpcState& asNpcState()
        {
            return *this;
        }
        virtual const NpcState& asNpcState() const
        {
            return *this;
        }
    };
}

#endif
