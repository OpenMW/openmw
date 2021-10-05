#include "spellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void SpellState::load(ESMReader &esm)
    {
        if(esm.getFormat() < 17)
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

                mSpellParams[id] = state;
                mSpells.emplace_back(id);
            }
        }
        else
        {
            while (esm.isNextSub("SPEL"))
                mSpells.emplace_back(esm.getHString());
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
        for (const std::string& spell : mSpells)
            esm.writeHNString("SPEL", spell);

        for (std::map<std::string, TimeStamp>::const_iterator it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
        {
            esm.writeHNString("USED", it->first);
            esm.writeHNT("TIME", it->second);
        }

        if (!mSelectedSpell.empty())
            esm.writeHNString("SLCT", mSelectedSpell);
    }

}
