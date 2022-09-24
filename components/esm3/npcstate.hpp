#ifndef OPENMW_ESM_NPCSTATE_H
#define OPENMW_ESM_NPCSTATE_H

#include "creaturestats.hpp"
#include "inventorystate.hpp"
#include "npcstats.hpp"
#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct NpcState final : public ObjectState
    {
        InventoryState mInventory;
        NpcStats mNpcStats;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank() override;

        void load(ESMReader& esm) override;
        void save(ESMWriter& esm, bool inInventory = false) const override;

        NpcState& asNpcState() override { return *this; }
        const NpcState& asNpcState() const override { return *this; }
    };
}

#endif
