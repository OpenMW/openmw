#include "activespells.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void ActiveSpells::save(ESMWriter &esm) const
    {
        for (TContainer::const_iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            esm.writeHNString ("ID__", it->first);

            const ActiveSpellParams& params = it->second;

            esm.writeHNT ("CAST", params.mCasterActorId);
            esm.writeHNString ("DISP", params.mDisplayName);
            esm.writeHNT ("TIME", params.mTimeStamp);

            for (std::vector<ActiveEffect>::const_iterator effectIt = params.mEffects.begin(); effectIt != params.mEffects.end(); ++effectIt)
            {
                esm.writeHNT ("MGEF", effectIt->mEffectId);
                if (effectIt->mArg != -1)
                    esm.writeHNT ("ARG_", effectIt->mArg);
                esm.writeHNT ("MAGN", effectIt->mMagnitude);
                esm.writeHNT ("DURA", effectIt->mDuration);
            }
        }
    }

    void ActiveSpells::load(ESMReader &esm)
    {
        while (esm.isNextSub("ID__"))
        {
            std::string spellId = esm.getHString();

            ActiveSpellParams params;
            esm.getHNT (params.mCasterActorId, "CAST");
            params.mDisplayName = esm.getHNString ("DISP");
            esm.getHNT (params.mTimeStamp, "TIME");

            while (esm.isNextSub("MGEF"))
            {
                ActiveEffect effect;
                esm.getHT(effect.mEffectId);
                effect.mArg = -1;
                esm.getHNOT(effect.mArg, "ARG_");
                esm.getHNT (effect.mMagnitude, "MAGN");
                esm.getHNT (effect.mDuration, "DURA");
                params.mEffects.push_back(effect);
            }
            mSpells.insert(std::make_pair(spellId, params));
        }
    }
}
