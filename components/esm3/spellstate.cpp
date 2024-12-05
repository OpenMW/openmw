#include "spellstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void SpellState::load(ESMReader& esm)
    {
        if (esm.getFormatVersion() <= MaxClearModifiersFormatVersion)
        {
            while (esm.isNextSub("SPEL"))
            {
                ESM::RefId id = esm.getRefId();

                SpellParams state;
                while (esm.isNextSub("INDX"))
                {
                    int32_t index;
                    esm.getHT(index);

                    float magnitude;
                    esm.getHNT(magnitude, "RAND");

                    state.mEffectRands[index] = magnitude;
                }

                while (esm.isNextSub("PURG"))
                {
                    int32_t index;
                    esm.getHT(index);
                    state.mPurgedEffects.insert(index);
                }

                mSpellParams[id] = std::move(state);
                mSpells.emplace_back(id);
            }
        }
        else
        {
            while (esm.isNextSub("SPEL"))
                mSpells.emplace_back(esm.getRefId());
        }

        // Obsolete
        while (esm.isNextSub("PERM"))
        {
            ESM::RefId spellId = esm.getRefId();
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
            mPermanentSpellEffects[spellId] = std::move(permEffectList);
        }

        // Obsolete
        while (esm.isNextSub("CORP"))
        {
            ESM::RefId id = esm.getRefId();

            CorprusStats stats;
            esm.getHNT(stats.mWorsenings, "WORS");
            stats.mNextWorsening.load(esm);

            mCorprusSpells[id] = stats;
        }

        while (esm.isNextSub("USED"))
        {
            ESM::RefId id = esm.getRefId();
            mUsedPowers[id].load(esm);
        }

        mSelectedSpell = esm.getHNORefId("SLCT");
    }

    void SpellState::save(ESMWriter& esm) const
    {
        for (const ESM::RefId& spell : mSpells)
            esm.writeHNRefId("SPEL", spell);

        for (auto it = mUsedPowers.begin(); it != mUsedPowers.end(); ++it)
        {
            esm.writeHNRefId("USED", it->first);
            esm.writeHNT("TIME", it->second);
        }

        if (!mSelectedSpell.empty())
            esm.writeHNRefId("SLCT", mSelectedSpell);
    }

}
