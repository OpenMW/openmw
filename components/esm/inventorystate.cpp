
#include "inventorystate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::InventoryState::load (ESMReader &esm)
{
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

    while (esm.isNextSub("LEVM"))
    {
        std::string id = esm.getHString();
        int count;
        esm.getHNT (count, "COUN");
        mLevelledItemMap[id] = count;
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
            params.push_back(std::make_pair(rand, multiplier));
        }
        mPermanentMagicEffectMagnitudes[id] = params;
    }

    while (esm.isNextSub("EQUI"))
    {
        esm.getSubHeader();
        int index;
        esm.getT(index);
        int slot;
        esm.getT(slot);
        mEquipmentSlots[index] = slot;
    }

    mSelectedEnchantItem = -1;
    esm.getHNOT(mSelectedEnchantItem, "SELE");
}

void ESM::InventoryState::save (ESMWriter &esm) const
{
    for (std::vector<ObjectState>::const_iterator iter (mItems.begin()); iter!=mItems.end(); ++iter)
    {
        int unused = 0;
        esm.writeHNT ("IOBJ", unused);

        iter->save (esm, true);
    }

    for (std::map<std::string, int>::const_iterator it = mLevelledItemMap.begin(); it != mLevelledItemMap.end(); ++it)
    {
        esm.writeHNString ("LEVM", it->first);
        esm.writeHNT ("COUN", it->second);
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

    for (std::map<int, int>::const_iterator it = mEquipmentSlots.begin(); it != mEquipmentSlots.end(); ++it)
    {
        esm.startSubRecord("EQUI");
        esm.writeT(it->first);
        esm.writeT(it->second);
        esm.endRecord("EQUI");
    }

    if (mSelectedEnchantItem != -1)
        esm.writeHNT ("SELE", mSelectedEnchantItem);
}
