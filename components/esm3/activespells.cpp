#include "activespells.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "loadmgef.hpp"
#include "loadskil.hpp"

#include <components/esm/attr.hpp>

#include <cstdint>

namespace ESM
{
    namespace
    {
        bool isSummon(const ESM::RefId& effectId)
        {
            static const std::array summonEffects{
                MagicEffect::SummonScamp,
                MagicEffect::SummonClannfear,
                MagicEffect::SummonDaedroth,
                MagicEffect::SummonDremora,
                MagicEffect::SummonAncestralGhost,
                MagicEffect::SummonSkeletalMinion,
                MagicEffect::SummonBonewalker,
                MagicEffect::SummonGreaterBonewalker,
                MagicEffect::SummonBonelord,
                MagicEffect::SummonWingedTwilight,
                MagicEffect::SummonHunger,
                MagicEffect::SummonGoldenSaint,
                MagicEffect::SummonFlameAtronach,
                MagicEffect::SummonFrostAtronach,
                MagicEffect::SummonStormAtronach,
                MagicEffect::SummonCenturionSphere,
                MagicEffect::SummonFabricant,
                MagicEffect::SummonWolf,
                MagicEffect::SummonBear,
                MagicEffect::SummonBonewolf,
                MagicEffect::SummonCreature04,
                MagicEffect::SummonCreature05,
            };
            return std::find(summonEffects.begin(), summonEffects.end(), effectId) != summonEffects.end();
        }
        bool affectsAttribute(const ESM::RefId& effectId)
        {
            static const std::array affectsAttributeEffects{
                MagicEffect::DrainAttribute,
                MagicEffect::DamageAttribute,
                MagicEffect::RestoreAttribute,
                MagicEffect::FortifyAttribute,
                MagicEffect::AbsorbAttribute,
            };
            return std::find(affectsAttributeEffects.begin(), affectsAttributeEffects.end(), effectId)
                != affectsAttributeEffects.end();
        }
        bool affectsSkill(const ESM::RefId& effectId)
        {
            static const std::array affectsSkillEffects{
                MagicEffect::DrainSkill,
                MagicEffect::DamageSkill,
                MagicEffect::RestoreSkill,
                MagicEffect::FortifySkill,
                MagicEffect::AbsorbSkill,
            };
            if (effectId.empty())
                return false;
            for (size_t i = 0; i < affectsSkillEffects.size(); ++i)
                if (affectsSkillEffects[i] == effectId)
                    return true;
            return false;
        }

        void saveImpl(ESMWriter& esm, const std::vector<ActiveSpells::ActiveSpellParams>& spells, NAME tag)
        {
            for (const auto& params : spells)
            {
                esm.writeHNRefId(tag, params.mSourceSpellId);
                esm.writeHNRefId("SPID", params.mActiveSpellId);

                esm.writeFormId(params.mCaster, true, "CAST");
                esm.writeHNString("DISP", params.mDisplayName);
                esm.writeHNT("FLAG", params.mFlags);
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
                    if (const ESM::RefId* id = std::get_if<ESM::RefId>(&effect.mArg))
                    {
                        if (!id->empty())
                            esm.writeHNRefId("ARG_", *id);
                    }
                    else if (const ESM::RefNum* actor = std::get_if<ESM::RefNum>(&effect.mArg))
                    {
                        if (actor->isSet())
                            esm.writeFormId(*actor, true, "SUM_");
                    }
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
                params.mSourceSpellId = esm.getRefId();
                if (format > MaxActiveSpellTypeVersion)
                    params.mActiveSpellId = esm.getHNRefId("SPID");
                if (format <= MaxActorIdSaveGameFormatVersion)
                    esm.getHNT(params.mCaster.mIndex, "CAST");
                else
                    params.mCaster = esm.getFormId(true, "CAST");
                params.mDisplayName = esm.getHNString("DISP");
                if (format <= MaxClearModifiersFormatVersion)
                    params.mFlags = Compatibility::ActiveSpells::Type_Temporary_Flags;
                else
                {
                    if (format <= MaxActiveSpellTypeVersion)
                    {
                        Compatibility::ActiveSpells::EffectType type;
                        esm.getHNT(type, "TYPE");
                        switch (type)
                        {
                            case Compatibility::ActiveSpells::Type_Ability:
                                params.mFlags = Compatibility::ActiveSpells::Type_Ability_Flags;
                                break;
                            case Compatibility::ActiveSpells::Type_Consumable:
                                params.mFlags = Compatibility::ActiveSpells::Type_Consumable_Flags;
                                break;
                            case Compatibility::ActiveSpells::Type_Enchantment:
                                params.mFlags = Compatibility::ActiveSpells::Type_Enchantment_Flags;
                                break;
                            case Compatibility::ActiveSpells::Type_Permanent:
                                params.mFlags = Compatibility::ActiveSpells::Type_Permanent_Flags;
                                break;
                            case Compatibility::ActiveSpells::Type_Temporary:
                                params.mFlags = Compatibility::ActiveSpells::Type_Temporary_Flags;
                                break;
                        }
                    }
                    else
                    {
                        esm.getHNT(params.mFlags, "FLAG");
                    }
                    if (esm.peekNextSub("ITEM"))
                        params.mItem = esm.getFormId(true, "ITEM");
                }
                if (esm.isNextSub("WORS"))
                {
                    esm.getHT(params.mWorsenings);
                    params.mNextWorsening.load(esm);
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
                    if (format <= MaxActorIdSaveGameFormatVersion)
                    {
                        int32_t arg = -1;
                        esm.getHNOT(arg, "ARG_");
                        if (arg >= 0)
                        {
                            if (isSummon(effect.mEffectId))
                                effect.mArg = RefNum{ .mIndex = static_cast<uint32_t>(arg), .mContentFile = -1 };
                            else if (affectsAttribute(effect.mEffectId))
                                effect.mArg = ESM::Attribute::indexToRefId(arg);
                            else if (affectsSkill(effect.mEffectId))
                                effect.mArg = ESM::Skill::indexToRefId(arg);
                        }
                    }
                    else if (esm.peekNextSub("ARG_"))
                        effect.mArg = esm.getHNRefId("ARG_");
                    else if (esm.peekNextSub("SUM_"))
                        effect.mArg = esm.getFormId(true, "SUM_");
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

    void ActiveSpells::save(ESMWriter& esm) const
    {
        saveImpl(esm, mSpells, "ID__");
        saveImpl(esm, mQueue, "QID_");
    }

    void ActiveSpells::load(ESMReader& esm)
    {
        mActorIdConverter = esm.getActorIdConverter();
        loadImpl(esm, mSpells, "ID__");
        loadImpl(esm, mQueue, "QID_");
    }

    RefId ActiveEffect::getSkillOrAttribute() const
    {
        if (const ESM::RefId* id = std::get_if<ESM::RefId>(&mArg))
            return *id;
        return {};
    }

    RefNum ActiveEffect::getActor() const
    {
        if (const ESM::RefNum* actor = std::get_if<ESM::RefNum>(&mArg))
            return *actor;
        return {};
    }
}
