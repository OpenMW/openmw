
#include "inventorystate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
    void read (ESM::ESMReader &esm, ESM::ObjectState& state, int& slot)
    {
        slot = -1;
        esm.getHNOT (slot, "SLOT");

        state.mRef.loadId(esm, true);
        state.load (esm);
    }

    void write (ESM::ESMWriter &esm, const ESM::ObjectState& state, int slot)
    {
        int unused = 0;
        esm.writeHNT ("IOBJ", unused);

        if (slot!=-1)
            esm.writeHNT ("SLOT", slot);

        state.save (esm, true);
    }
}

void ESM::InventoryState::load (ESMReader &esm)
{
    while (esm.isNextSub ("IOBJ"))
    {
        int unused; // no longer used
        esm.getHT(unused);

        ObjectState state;
        int slot;
        read (esm, state, slot);
        if (state.mCount == 0)
            continue;
        mItems.push_back (std::make_pair (state, slot));
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
}

void ESM::InventoryState::save (ESMWriter &esm) const
{
    for (std::vector<std::pair<ObjectState, int> >::const_iterator iter (mItems.begin()); iter!=mItems.end(); ++iter)
        write (esm, iter->first, iter->second);

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
}
