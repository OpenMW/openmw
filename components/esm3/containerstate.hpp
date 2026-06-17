#ifndef OPENMW_ESM_CONTAINERSTATE_H
#define OPENMW_ESM_CONTAINERSTATE_H

#include "inventorystate.hpp"
#include "objectstate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct ContainerState final : public ObjectState
    {
        InventoryState mInventory;

        void load(ESMReader& esm) override
        {
            ObjectState::load(esm);

            mInventory.load(esm);
        }

        void save(ESMWriter& esm, bool inInventory = false) const override
        {
            ObjectState::save(esm, inInventory);

            mInventory.save(esm);
        }

        ContainerState& asContainerState() override { return *this; }
        const ContainerState& asContainerState() const override { return *this; }
    };
}

#endif
