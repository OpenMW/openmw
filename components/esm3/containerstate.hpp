#ifndef OPENMW_ESM_CONTAINERSTATE_H
#define OPENMW_ESM_CONTAINERSTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct ContainerState final : public ObjectState
    {
        InventoryState mInventory;

        void load (ESMReader &esm) override;
        void save (ESMWriter &esm, bool inInventory = false) const override;

        ContainerState& asContainerState() override
        {
            return *this;
        }
        const ContainerState& asContainerState() const override
        {
            return *this;
        }
    };
}

#endif
