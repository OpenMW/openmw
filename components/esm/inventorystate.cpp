
#include "inventorystate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
    void read (ESM::ESMReader &esm, ESM::ObjectState& state, int& slot)
    {
        slot = -1;
        esm.getHNOT (slot, "SLOT");

        state.load (esm);
    }

    void write (ESM::ESMWriter &esm, const ESM::ObjectState& state, unsigned int type, int slot)
    {
        esm.writeHNT ("IOBJ", type);

        if (slot!=-1)
            esm.writeHNT ("SLOT", slot);

        state.save (esm, true);
    }
}

void ESM::InventoryState::load (ESMReader &esm)
{
    while (esm.isNextSub ("IOBJ"))
    {
        unsigned int id = 0;
        esm.getHT (id);

        if (id==ESM::REC_LIGH)
        {
            LightState state;
            int slot;
            read (esm, state, slot);
            if (state.mCount == 0)
                continue;
            mLights.push_back (std::make_pair (state, slot));
        }
        else
        {
            ObjectState state;
            int slot;
            read (esm, state, slot);
            if (state.mCount == 0)
                continue;
            mItems.push_back (std::make_pair (state, std::make_pair (id, slot)));
        }
    }

    while (esm.isNextSub("LEVM"))
    {
        std::string id = esm.getHString();
        int count;
        esm.getHNT (count, "COUN");
        mLevelledItemMap[id] = count;
    }
}

void ESM::InventoryState::save (ESMWriter &esm) const
{
    for (std::vector<std::pair<ObjectState, std::pair<unsigned int, int> > >::const_iterator iter (mItems.begin()); iter!=mItems.end(); ++iter)
        write (esm, iter->first, iter->second.first, iter->second.second);

    for (std::vector<std::pair<LightState, int> >::const_iterator iter (mLights.begin());
        iter!=mLights.end(); ++iter)
        write (esm, iter->first, ESM::REC_LIGH, iter->second);

    for (std::map<std::string, int>::const_iterator it = mLevelledItemMap.begin(); it != mLevelledItemMap.end(); ++it)
    {
        esm.writeHNString ("LEVM", it->first);
        esm.writeHNT ("COUN", it->second);
    }
}
