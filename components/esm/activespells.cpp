#include "activespells.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
    void save(ESM::ESMWriter& esm, const std::vector<ESM::ActiveSpells::ActiveSpellParams>& spells, const std::string& tag)
    {
        for (const auto& params : spells)
        {
            esm.writeHNString (tag, params.mId);

            esm.writeHNT ("CAST", params.mCasterActorId);
            esm.writeHNString ("DISP", params.mDisplayName);
            esm.writeHNT ("TYPE", params.mType);
            if(params.mItem.isSet())
                params.mItem.save(esm, true, "ITEM");
            if(params.mWorsenings >= 0)
            {
                esm.writeHNT ("WORS", params.mWorsenings);
                esm.writeHNT ("TIME", params.mNextWorsening);
            }

            for (auto effect : params.mEffects)
            {
                esm.writeHNT ("MGEF", effect.mEffectId);
                if (effect.mArg != -1)
                    esm.writeHNT ("ARG_", effect.mArg);
                esm.writeHNT ("MAGN", effect.mMagnitude);
                esm.writeHNT ("MAGN", effect.mMinMagnitude);
                esm.writeHNT ("MAGN", effect.mMaxMagnitude);
                esm.writeHNT ("DURA", effect.mDuration);
                esm.writeHNT ("EIND", effect.mEffectIndex);
                esm.writeHNT ("LEFT", effect.mTimeLeft);
                esm.writeHNT ("FLAG", effect.mFlags);
            }
        }
    }

    void load(ESM::ESMReader& esm, std::vector<ESM::ActiveSpells::ActiveSpellParams>& spells, const char* tag)
    {
        int format = esm.getFormat();

        while (esm.isNextSub(tag))
        {
            ESM::ActiveSpells::ActiveSpellParams params;
            params.mId = esm.getHString();
            esm.getHNT (params.mCasterActorId, "CAST");
            params.mDisplayName = esm.getHNString ("DISP");
            params.mItem.unset();
            if (format < 17)
                params.mType = ESM::ActiveSpells::Type_Temporary;
            else
            {
                esm.getHNT (params.mType, "TYPE");
                if(esm.peekNextSub("ITEM"))
                    params.mItem.load(esm, true, "ITEM");
            }
            if(esm.isNextSub("WORS"))
            {
                esm.getHT(params.mWorsenings);
                esm.getHNT(params.mNextWorsening, "TIME");
            }
            else
                params.mWorsenings = -1;

            // spell casting timestamp, no longer used
            if (esm.isNextSub("TIME"))
                esm.skipHSub();

            while (esm.isNextSub("MGEF"))
            {
                ESM::ActiveEffect effect;
                esm.getHT(effect.mEffectId);
                effect.mArg = -1;
                esm.getHNOT(effect.mArg, "ARG_");
                esm.getHNT (effect.mMagnitude, "MAGN");
                if (format < 17)
                {
                    effect.mMinMagnitude = effect.mMagnitude;
                    effect.mMaxMagnitude = effect.mMagnitude;
                }
                else
                {
                    esm.getHNT (effect.mMinMagnitude, "MAGN");
                    esm.getHNT (effect.mMaxMagnitude, "MAGN");
                }
                esm.getHNT (effect.mDuration, "DURA");
                effect.mEffectIndex = -1;
                esm.getHNOT (effect.mEffectIndex, "EIND");
                if (format < 9)
                    effect.mTimeLeft = effect.mDuration;
                else
                    esm.getHNT (effect.mTimeLeft, "LEFT");
                if (format < 17)
                    effect.mFlags = ESM::ActiveEffect::Flag_None;
                else
                    esm.getHNT (effect.mFlags, "FLAG");

                params.mEffects.push_back(effect);
            }
            spells.emplace_back(params);
        }
    }
}

namespace ESM
{

    void ActiveSpells::save(ESMWriter &esm) const
    {
        ::save(esm, mSpells, "ID__");
        ::save(esm, mQueue, "QID_");
    }

    void ActiveSpells::load(ESMReader &esm)
    {
        ::load(esm, mSpells, "ID__");
        ::load(esm, mQueue, "QID_");
    }
}
