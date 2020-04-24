#ifndef OPENMW_ESM_CONTAINERSTATE_H
#define OPENMW_ESM_CONTAINERSTATE_H

#include "objectstate.hpp"
#include "inventorystate.hpp"

namespace ESM
{
    // format 0, saved games only

    struct ContainerState : public ObjectState
    {
        InventoryState mInventory;

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm, bool inInventory = false) const;

        virtual ContainerState& asContainerState()
        {
            return *this;
        }
        virtual const ContainerState& asContainerState() const
        {
            return *this;
        }
    };
}

#endif
