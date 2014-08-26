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

        while (esm.isNextSub("PERM"))
        {
            std::string spellId = esm.getHString();

            std::vector<PermanentSpellEffectInfo> permEffectList;
            while (esm.isNextSub("EFID"))
            {
                PermanentSpellEffectInfo info;
                esm.getHT(info.mId);
                esm.getHNT(info.mArg, "ARG_");
                esm.getHNT(info.mMagnitude, "MAGN");

                permEffectList.push_back(info);
            }
            mPermanentSpellEffects[spellId] = permEffectList;
        }

        while (esm.isNextSub("CORP"))
        {
            std::string id = esm.getHString();

            CorprusStats stats;
            esm.getHNT(stats.mWorsenings, "WORS");
            esm.getHNT(stats.mNextWorsening, "TIME");

            mCorprusSpells[id] = stats;
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

        for (std::map<std::string, std::vector<PermanentSpellEffectInfo> >::const_iterator it = mPermanentSpellEffects.begin(); it != mPermanentSpellEffects.end(); ++it)
        {
            esm.writeHNString("PERM", it->first);

            const std::vector<PermanentSpellEffectInfo> & effects = it->second;
            for (std::vector<PermanentSpellEffectInfo>::const_iterator effectIt = effects.begin(); effectIt != effects.end(); ++effectIt)
            {
                esm.writeHNT("EFID", effectIt->mId);
                esm.writeHNT("ARG_", effectIt->mArg);
                esm.writeHNT("MAGN", effectIt->mMagnitude);
            }
        }

        for (std::map<std::string, CorprusStats>::const_iterator it = mCorprusSpells.begin(); it != mCorprusSpells.end(); ++it)
        {
            esm.writeHNString("CORP", it->first);

            const CorprusStats & stats = it->second;
            esm.writeHNT("WORS", stats.mWorsenings);
            esm.writeHNT("TIME", stats.mNextWorsening);
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
