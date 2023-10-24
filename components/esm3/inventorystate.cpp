#include "inventorystate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
    namespace
    {
        constexpr uint32_t sInvalidSlot = static_cast<uint32_t>(-1);
    }

    void InventoryState::load(ESMReader& esm)
    {
        // obsolete
        uint32_t index = 0;
        while (esm.isNextSub("IOBJ"))
        {
            esm.skipHT<int32_t>();

            ObjectState state;

            // obsolete
            if (esm.isNextSub("SLOT"))
            {
                int32_t slot;
                esm.getHT(slot);
                mEquipmentSlots[index] = slot;
            }

            state.mRef.loadId(esm, true);
            state.load(esm);

            if (state.mCount == 0)
                continue;

            mItems.push_back(state);

            ++index;
        }

        uint32_t itemsCount = 0;
        esm.getHNOT(itemsCount, "ICNT");
        for (; itemsCount > 0; --itemsCount)
        {
            ObjectState state;

            state.mRef.loadId(esm, true);
            state.load(esm);

            // Update content file index if load order was changed.
            if (!esm.applyContentFileMapping(state.mRef.mRefNum))
                state.mRef.mRefNum = FormId(); // content file removed; unset refnum, but keep object.

            if (state.mCount == 0)
                continue;

            mItems.push_back(state);
        }

        // Next item is Levelled item
        while (esm.isNextSub("LEVM"))
        {
            // Get its name
            ESM::RefId id = esm.getRefId();
            int32_t count;
            std::string parentGroup;
            // Then get its count
            esm.getHNT(count, "COUN");
            // Old save formats don't have information about parent group; check for that
            if (esm.isNextSub("LGRP"))
                // Newest saves contain parent group
                parentGroup = esm.getHString();
            mLevelledItemMap[std::make_pair(id, parentGroup)] = count;
        }

        while (esm.isNextSub("MAGI"))
        {
            ESM::RefId id = esm.getRefId();

            std::vector<std::pair<float, float>> params;
            while (esm.isNextSub("RAND"))
            {
                float rand, multiplier;
                esm.getHT(rand);
                esm.getHNT(multiplier, "MULT");
                params.emplace_back(rand, multiplier);
            }
            mPermanentMagicEffectMagnitudes[id] = params;
        }

        while (esm.isNextSub("EQUI"))
        {
            esm.getSubHeader();
            int32_t equipIndex;
            esm.getT(equipIndex);
            int32_t slot;
            esm.getT(slot);
            mEquipmentSlots[equipIndex] = slot;
        }

        if (esm.isNextSub("EQIP"))
        {
            esm.getSubHeader();
            uint32_t slotsCount = 0;
            esm.getT(slotsCount);
            for (; slotsCount > 0; --slotsCount)
            {
                int32_t equipIndex;
                esm.getT(equipIndex);
                int32_t slot;
                esm.getT(slot);
                mEquipmentSlots[equipIndex] = slot;
            }
        }

        uint32_t selectedEnchantItem = sInvalidSlot;
        esm.getHNOT(selectedEnchantItem, "SELE");
        if (selectedEnchantItem == sInvalidSlot)
            mSelectedEnchantItem.reset();
        else
            mSelectedEnchantItem = selectedEnchantItem;

        // Old saves had restocking levelled items in a special map
        // This turns items from that map into negative quantities
        for (const auto& entry : mLevelledItemMap)
        {
            const ESM::RefId& id = entry.first.first;
            const int count = entry.second;
            for (auto& item : mItems)
            {
                if (item.mCount == count && id == item.mRef.mRefID)
                    item.mCount = -count;
            }
        }
    }

    void InventoryState::save(ESMWriter& esm) const
    {
        uint32_t itemsCount = static_cast<uint32_t>(mItems.size());
        if (itemsCount > 0)
        {
            esm.writeHNT("ICNT", itemsCount);
            for (const ObjectState& state : mItems)
            {
                state.save(esm, true);
            }
        }

        for (auto it = mLevelledItemMap.begin(); it != mLevelledItemMap.end(); ++it)
        {
            esm.writeHNRefId("LEVM", it->first.first);
            esm.writeHNT("COUN", it->second);
            esm.writeHNString("LGRP", it->first.second);
        }

        for (const auto& [id, params] : mPermanentMagicEffectMagnitudes)
        {
            esm.writeHNRefId("MAGI", id);

            for (const auto& [rand, mult] : params)
            {
                esm.writeHNT("RAND", rand);
                esm.writeHNT("MULT", mult);
            }
        }

        uint32_t slotsCount = static_cast<uint32_t>(mEquipmentSlots.size());
        if (slotsCount > 0)
        {
            esm.startSubRecord("EQIP");
            esm.writeT(slotsCount);
            for (const auto& [index, slot] : mEquipmentSlots)
            {
                esm.writeT(index);
                esm.writeT(slot);
            }
            esm.endRecord("EQIP");
        }

        if (mSelectedEnchantItem)
            esm.writeHNT("SELE", *mSelectedEnchantItem);
    }

}
