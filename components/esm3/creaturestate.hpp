#ifndef OPENMW_ESM_CREATURESTATE_H
#define OPENMW_ESM_CREATURESTATE_H

#include "creaturestats.hpp"
#include "inventorystate.hpp"
#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct CreatureState final : public ObjectState
    {
        InventoryState mInventory;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank() override;

        void load(ESMReader& esm) override;
        void save(ESMWriter& esm, bool inInventory = false) const override;

        CreatureState& asCreatureState() override { return *this; }
        const CreatureState& asCreatureState() const override { return *this; }
    };
}

#endif
