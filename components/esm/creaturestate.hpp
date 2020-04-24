#ifndef OPENMW_ESM_CREATURESTATE_H
#define OPENMW_ESM_CREATURESTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"
#include "creaturestats.hpp"

namespace ESM
{
    // format 0, saved games only

    struct CreatureState : public ObjectState
    {
        InventoryState mInventory;
        CreatureStats mCreatureStats;

        /// Initialize to default state
        void blank();

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;

        virtual CreatureState& asCreatureState()
        {
            return *this;
        }
        virtual const CreatureState& asCreatureState() const
        {
            return *this;
        }
    };
}

#endif
