#include "activespells.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    namespace
    {
        void saveImpl(ESMWriter& esm, const std::vector<ActiveSpells::ActiveSpellParams>& spells, NAME tag)
        {
            for (const auto& params : spells)
            {
                esm.writeHNRefId(tag, params.mId);

                esm.writeHNT("CAST", params.mCasterActorId);
                esm.writeHNString("DISP", params.mDisplayName);
                esm.writeHNT("TYPE", params.mType);
                if (params.mItem.isSet())
                    esm.writeFormId(params.mItem, true, "ITEM");
                if (params.mWorsenings >= 0)
                {
                    esm.writeHNT("WORS", params.mWorsenings);
                    esm.writeHNT("TIME", params.mNextWorsening);
                }

                for (auto& effect : params.mEffects)
                {
                    esm.writeHNT("MGEF", effect.mEffectId);
                    if (effect.mArg != -1)
                        esm.writeHNT("ARG_", effect.mArg);
                    esm.writeHNT("MAGN", effect.mMagnitude);
                    esm.writeHNT("MAGN", effect.mMinMagnitude);
                    esm.writeHNT("MAGN", effect.mMaxMagnitude);
                    esm.writeHNT("DURA", effect.mDuration);
                    esm.writeHNT("EIND", effect.mEffectIndex);
                    esm.writeHNT("LEFT", effect.mTimeLeft);
                    esm.writeHNT("FLAG", effect.mFlags);
                }
            }
        }

        void loadImpl(ESMReader& esm, std::vector<ActiveSpells::ActiveSpellParams>& spells, NAME tag)
        {
            const FormatVersion format = esm.getFormatVersion();

            while (esm.isNextSub(tag))
            {
                ActiveSpells::ActiveSpellParams params;
                params.mId = esm.getRefId();
                esm.getHNT(params.mCasterActorId, "CAST");
                params.mDisplayName = esm.getHNString("DISP");
                if (format <= MaxClearModifiersFormatVersion)
                    params.mType = ActiveSpells::Type_Temporary;
                else
                {
                    esm.getHNT(params.mType, "TYPE");
                    if (esm.peekNextSub("ITEM"))
                    {
                        if (format <= MaxActiveSpellSlotIndexFormatVersion)
                            // Previous versions saved slot index in this record.
                            // Ignore these values as we can't use them
                            esm.getFormId(true, "ITEM");
                        else
                            params.mItem = esm.getFormId(true, "ITEM");
                    }
                }
                if (esm.isNextSub("WORS"))
                {
                    esm.getHT(params.mWorsenings);
                    esm.getHNTSized<8>(params.mNextWorsening, "TIME");
                }
                else
                    params.mWorsenings = -1;

                // spell casting timestamp, no longer used
                if (esm.isNextSub("TIME"))
                    esm.skipHSub();

                while (esm.isNextSub("MGEF"))
                {
                    ActiveEffect effect;
                    esm.getHT(effect.mEffectId);
                    effect.mArg = -1;
                    esm.getHNOT(effect.mArg, "ARG_");
                    esm.getHNT(effect.mMagnitude, "MAGN");
                    if (format <= MaxClearModifiersFormatVersion)
                    {
                        effect.mMinMagnitude = effect.mMagnitude;
                        effect.mMaxMagnitude = effect.mMagnitude;
                    }
                    else
                    {
                        esm.getHNT(effect.mMinMagnitude, "MAGN");
                        esm.getHNT(effect.mMaxMagnitude, "MAGN");
                    }
                    esm.getHNT(effect.mDuration, "DURA");
                    effect.mEffectIndex = -1;
                    esm.getHNOT(effect.mEffectIndex, "EIND");
                    if (format <= MaxOldTimeLeftFormatVersion)
                        effect.mTimeLeft = effect.mDuration;
                    else
                        esm.getHNT(effect.mTimeLeft, "LEFT");
                    if (format <= MaxClearModifiersFormatVersion)
                        effect.mFlags = ActiveEffect::Flag_None;
                    else
                        esm.getHNT(effect.mFlags, "FLAG");

                    params.mEffects.push_back(effect);
                }
                spells.emplace_back(params);
            }
        }
    }
}

namespace ESM
{
    void ActiveSpells::save(ESMWriter& esm) const
    {
        saveImpl(esm, mSpells, "ID__");
        saveImpl(esm, mQueue, "QID_");
    }

    void ActiveSpells::load(ESMReader& esm)
    {
        loadImpl(esm, mSpells, "ID__");
        loadImpl(esm, mQueue, "QID_");
    }
}
