#include "inventorystate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/stringops.hpp>

void ESM::InventoryState::load (ESMReader &esm)
{
    // obsolete
    int index = 0;
    while (esm.isNextSub ("IOBJ"))
    {
        int unused; // no longer used
        esm.getHT(unused);

        ObjectState state;

        // obsolete
        if (esm.isNextSub("SLOT"))
        {
            int slot;
            esm.getHT(slot);
            mEquipmentSlots[index] = slot;
        }

        state.mRef.loadId(esm, true);
        state.load (esm);

        if (state.mCount == 0)
            continue;

        mItems.push_back (state);

        ++index;
    }

    int itemsCount = 0;
    esm.getHNOT(itemsCount, "ICNT");
    for (int i = 0; i < itemsCount; i++)
    {
        ObjectState state;

        state.mRef.loadId(esm, true);
        state.load (esm);

        if (state.mCount == 0)
            continue;

        mItems.push_back (state);
    }

    //Next item is Levelled item
    while (esm.isNextSub("LEVM"))
    {
        //Get its name
        std::string id = esm.getHString();
        int count;
        std::string parentGroup = "";
        //Then get its count
        esm.getHNT (count, "COUN");
        //Old save formats don't have information about parent group; check for that
        if(esm.isNextSub("LGRP"))
            //Newest saves contain parent group
            parentGroup = esm.getHString();
        mLevelledItemMap[std::make_pair(id, parentGroup)] = count;
    }

    while (esm.isNextSub("MAGI"))
    {
        std::string id = esm.getHString();

        std::vector<std::pair<float, float> > params;
        while (esm.isNextSub("RAND"))
        {
            float rand, multiplier;
            esm.getHT (rand);
            esm.getHNT (multiplier, "MULT");
            params.emplace_back(rand, multiplier);
        }
        mPermanentMagicEffectMagnitudes[id] = params;
    }

    while (esm.isNextSub("EQUI"))
    {
        esm.getSubHeader();
        int equipIndex;
        esm.getT(equipIndex);
        int slot;
        esm.getT(slot);
        mEquipmentSlots[equipIndex] = slot;
    }

    if (esm.isNextSub("EQIP"))
    {
        esm.getSubHeader();
        int slotsCount = 0;
        esm.getT(slotsCount);
        for (int i = 0; i < slotsCount; i++)
        {
            int equipIndex;
            esm.getT(equipIndex);
            int slot;
            esm.getT(slot);
            mEquipmentSlots[equipIndex] = slot;
        }
    }

    mSelectedEnchantItem = -1;
    esm.getHNOT(mSelectedEnchantItem, "SELE");

    // Old saves had restocking levelled items in a special map
    // This turns items from that map into negative quantities
    for(const auto& entry : mLevelledItemMap)
    {
        const std::string& id = entry.first.first;
        const int count = entry.second;
        for(auto& item : mItems)
        {
            if(item.mCount == count && Misc::StringUtils::ciEqual(id, item.mRef.mRefID))
                item.mCount = -count;
        }
    }
}

void ESM::InventoryState::save (ESMWriter &esm) const
{
    int itemsCount = static_cast<int>(mItems.size());
    if (itemsCount > 0)
    {
        esm.writeHNT ("ICNT", itemsCount);
        for (const ObjectState& state : mItems)
        {
            state.save (esm, true);
        }
    }

    for (std::map<std::pair<std::string, std::string>, int>::const_iterator it = mLevelledItemMap.begin(); it != mLevelledItemMap.end(); ++it)
    {
        esm.writeHNString ("LEVM", it->first.first);
        esm.writeHNT ("COUN", it->second);
        esm.writeHNString("LGRP", it->first.second);
    }

    for (TEffectMagnitudes::const_iterator it = mPermanentMagicEffectMagnitudes.begin(); it != mPermanentMagicEffectMagnitudes.end(); ++it)
    {
        esm.writeHNString("MAGI", it->first);

        const std::vector<std::pair<float, float> >& params = it->second;
        for (std::vector<std::pair<float, float> >::const_iterator pIt = params.begin(); pIt != params.end(); ++pIt)
        {
            esm.writeHNT ("RAND", pIt->first);
            esm.writeHNT ("MULT", pIt->second);
        }
    }

    int slotsCount = static_cast<int>(mEquipmentSlots.size());
    if (slotsCount > 0)
    {
        esm.startSubRecord("EQIP");
        esm.writeT(slotsCount);
        for (std::map<int, int>::const_iterator it = mEquipmentSlots.begin(); it != mEquipmentSlots.end(); ++it)
        {
            esm.writeT(it->first);
            esm.writeT(it->second);
        }
        esm.endRecord("EQIP");
    }

    if (mSelectedEnchantItem != -1)
        esm.writeHNT ("SELE", mSelectedEnchantItem);
}
