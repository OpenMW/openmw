#ifndef OPENMW_ESM_CREATURESTATE_H
#define OPENMW_ESM_CREATURESTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"
#include "creaturestats.hpp"

namespace ESM
{
    // format 0, saved games only

    struct CreatureState final : public ObjectState
    {
        InventoryState mInventory;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank();

        void load (ESMReader &esm) final;
        void save (ESMWriter &esm, bool inInventory = false) const final;

        CreatureState& asCreatureState() final
        {
            return *this;
        }
        const CreatureState& asCreatureState() const final
        {
            return *this;
        }
    };
}

#endif
