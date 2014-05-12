#include "spellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void SpellState::load(ESMReader &esm)
    {
        while (esm.isNextSub("SPEL"))
        {
            std::string id = esm.getHString();

            std::map<const int, float> random;
            while (esm.isNextSub("INDX"))
            {
                int index;
                esm.getHT(index);

                float magnitude;
                esm.getHNT(magnitude, "RAND");

                random[index] = magnitude;
            }

            mSpells[id] = random;
        }

        while (esm.isNextSub("USED"))
        {
            std::string id = esm.getHString();
            TimeStamp time;
            esm.getHNT(time, "TIME");

            mUsedPowers[id] = time;
        }

        mSelectedSpell = esm.getHNOString("SLCT");
    }

    void SpellState::save(ESMWriter &esm) const
    {
        for (TContainer::const_iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            esm.writeHNString("SPEL", it->first);

            const std::map<const int, float>& random = it->second;
            for (std::map<const int, float>::const_iterator rIt = random.begin(); rIt != random.end(); ++rIt)
            {
                esm.writeHNT("INDX", rIt->first);
                esm.writeHNT("RAND", rIt->second);
            }
        }

        for (std::map<std::string, TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
        {
            esm.writeHNString("USED", it->first);
            esm.writeHNT("TIME", it->second);
        }

        if (!mSelectedSpell.empty())
            esm.writeHNString("SLCT", mSelectedSpell);
    }

}
