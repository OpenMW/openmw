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

            SpellParams state;
            while (esm.isNextSub("INDX"))
            {
                int index;
                esm.getHT(index);

                float magnitude;
                esm.getHNT(magnitude, "RAND");

                state.mEffectRands[index] = magnitude;
            }

            while (esm.isNextSub("PURG")) {
                int index;
                esm.getHT(index);
                state.mPurgedEffects.insert(index);
            }

            mSpells[id] = state;
        }

        // Obsolete
        while (esm.isNextSub("PERM"))
        {
            std::string spellId = esm.getHString();
            std::vector<PermanentSpellEffectInfo> permEffectList;

            while (true)
            {
                ESM_Context restorePoint = esm.getContext();

                if (!esm.isNextSub("EFID"))
                    break;

                PermanentSpellEffectInfo info;
                esm.getHT(info.mId);
                if (esm.isNextSub("BASE"))
                {
                    esm.restoreContext(restorePoint);
                    return;
                }
                else
                    esm.getHNT(info.mArg, "ARG_");

                esm.getHNT(info.mMagnitude, "MAGN");
                permEffectList.push_back(info);
            }
            mPermanentSpellEffects[spellId] = permEffectList;
        }

        // Obsolete
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

            const std::map<int, float>& random = it->second.mEffectRands;
            for (std::map<int, float>::const_iterator rIt = random.begin(); rIt != random.end(); ++rIt)
            {
                esm.writeHNT("INDX", rIt->first);
                esm.writeHNT("RAND", rIt->second);
            }

            const std::set<int>& purges = it->second.mPurgedEffects;
            for (std::set<int>::const_iterator pIt = purges.begin(); pIt != purges.end(); ++pIt)
                esm.writeHNT("PURG", *pIt);
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
