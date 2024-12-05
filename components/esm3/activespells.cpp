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
        bool isSummon(int effectId)
        {
            switch (effectId)
            {
                case MagicEffect::SummonScamp:
                case MagicEffect::SummonClannfear:
                case MagicEffect::SummonDaedroth:
                case MagicEffect::SummonDremora:
                case MagicEffect::SummonAncestralGhost:
                case MagicEffect::SummonSkeletalMinion:
                case MagicEffect::SummonBonewalker:
                case MagicEffect::SummonGreaterBonewalker:
                case MagicEffect::SummonBonelord:
                case MagicEffect::SummonWingedTwilight:
                case MagicEffect::SummonHunger:
                case MagicEffect::SummonGoldenSaint:
                case MagicEffect::SummonFlameAtronach:
                case MagicEffect::SummonFrostAtronach:
                case MagicEffect::SummonStormAtronach:
                case MagicEffect::SummonCenturionSphere:
                case MagicEffect::SummonFabricant:
                case MagicEffect::SummonWolf:
                case MagicEffect::SummonBear:
                case MagicEffect::SummonBonewolf:
                case MagicEffect::SummonCreature04:
                case MagicEffect::SummonCreature05:
                    return true;
            }
            return false;
        }
        bool affectsAttribute(int effectId)
        {
            switch (effectId)
            {
                case MagicEffect::DrainAttribute:
                case MagicEffect::DamageAttribute:
                case MagicEffect::RestoreAttribute:
                case MagicEffect::FortifyAttribute:
                case MagicEffect::AbsorbAttribute:
                    return true;
            }
            return false;
        }
        bool affectsSkill(int effectId)
        {
            switch (effectId)
            {
                case MagicEffect::DrainSkill:
                case MagicEffect::DamageSkill:
                case MagicEffect::RestoreSkill:
                case MagicEffect::FortifySkill:
                case MagicEffect::AbsorbSkill:
                    return true;
            }
            return false;
        }

        struct ToInt
        {
            int effectId;

            int operator()(const ESM::RefId& id) const
            {
                if (!id.empty())
                {
                    if (affectsAttribute(effectId))
                        return ESM::Attribute::refIdToIndex(id);
                    else if (affectsSkill(effectId))
                        return ESM::Skill::refIdToIndex(id);
                }
                return -1;
            }

            int operator()(int actor) const { return actor; }
        };

        void saveImpl(ESMWriter& esm, const std::vector<ActiveSpells::ActiveSpellParams>& spells, NAME tag)
        {
            for (const auto& params : spells)
            {
                esm.writeHNRefId(tag, params.mSourceSpellId);
                esm.writeHNRefId("SPID", params.mActiveSpellId);

                esm.writeHNT("CAST", params.mCasterActorId);
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
                    int arg = std::visit(ToInt{ effect.mEffectId }, effect.mArg);
                    if (arg != -1)
                        esm.writeHNT("ARG_", arg);
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
                esm.getHNT(params.mCasterActorId, "CAST");
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
                    int32_t arg = -1;
                    esm.getHNOT(arg, "ARG_");
                    if (arg >= 0)
                    {
                        if (isSummon(effect.mEffectId))
                            effect.mArg = arg;
                        else if (affectsAttribute(effect.mEffectId))
                            effect.mArg = ESM::Attribute::indexToRefId(arg);
                        else if (affectsSkill(effect.mEffectId))
                            effect.mArg = ESM::Skill::indexToRefId(arg);
                    }
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
        loadImpl(esm, mSpells, "ID__");
        loadImpl(esm, mQueue, "QID_");
    }

    RefId ActiveEffect::getSkillOrAttribute() const
    {
        if (const auto* id = std::get_if<ESM::RefId>(&mArg))
            return *id;
        return {};
    }

    int ActiveEffect::getActorId() const
    {
        if (const auto* id = std::get_if<int>(&mArg))
            return *id;
        return -1;
    }
}
