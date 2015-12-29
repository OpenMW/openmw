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
        std::vector<ObjectState> mItems;

        // <Index in mItems, equipment slot>
        std::map<int, int> mEquipmentSlots;

        std::map<std::pair<std::string, std::string>, int> mLevelledItemMap;

        typedef std::map<std::string, std::vector<std::pair<float, float> > > TEffectMagnitudes;
        TEffectMagnitudes mPermanentMagicEffectMagnitudes;

        int mSelectedEnchantItem; // For inventories only

        InventoryState() : mSelectedEnchantItem(-1) {}
        virtual ~InventoryState() {}

        virtual void load (ESMReader &esm);
        virtual void save (ESMWriter &esm) const;
    };
}

#endif
