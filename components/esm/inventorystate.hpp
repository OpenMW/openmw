#ifndef OPENMW_ESM_INVENTORYSTATE_H
#define OPENMW_ESM_INVENTORYSTATE_H

#include <map>

#include "objectstate.hpp"
#include "lightstate.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    /// \brief State for inventories and containers
    struct InventoryState
    {
        // anything but lights (type, slot)
        std::vector<std::pair<ObjectState, std::pair<unsigned int, int> > > mItems;

        // lights (slot)
        std::vector<std::pair<LightState, int> > mLights;

        std::map<std::string, int> mLevelledItemMap;

        virtual ~InventoryState() {}

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm) const;
    };
}

#endif
