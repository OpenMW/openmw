#ifndef OPENMW_ESM_INVENTORYSTATE_H
#define OPENMW_ESM_INVENTORYSTATE_H

#include <map>

#include "objectstate.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    /// \brief State for inventories and containers
    struct InventoryState
    {
        std::vector<std::pair<ObjectState, int> > mItems;

        std::map<std::string, int> mLevelledItemMap;

        typedef std::map<std::string, std::vector<std::pair<float, float> > > TEffectMagnitudes;
        TEffectMagnitudes mPermanentMagicEffectMagnitudes;

        virtual ~InventoryState() {}

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm) const;
    };
}

#endif
