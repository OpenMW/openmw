#ifndef OPENMW_ESM_NPCSTATE_H
#define OPENMW_ESM_NPCSTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"

namespace ESM
{
    // format 0, saved games only

    struct NpcState final : public ObjectState
    {
        InventoryState mInventory;
        NpcStats mNpcStats;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank();

        void load (ESMReader &esm) final;
        void save (ESMWriter &esm, bool inInventory = false) const final;

        NpcState& asNpcState() final
        {
            return *this;
        }
        const NpcState& asNpcState() const final
        {
            return *this;
        }
    };
}

#endif
