#ifndef OPENMW_ESM_INVENTORYSTATE_H
#define OPENMW_ESM_INVENTORYSTATE_H

#include <map>
#include <optional>

#include "objectstate.hpp"
#include <components/esm/refid.hpp>

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
        std::map<uint32_t, int32_t> mEquipmentSlots;

        std::map<ESM::RefId, std::vector<std::pair<float, float>>> mPermanentMagicEffectMagnitudes;

        std::optional<uint32_t> mSelectedEnchantItem; // For inventories only

        virtual ~InventoryState() = default;

        virtual void load(ESMReader& esm);
        virtual void save(ESMWriter& esm) const;
    };
}

#endif
