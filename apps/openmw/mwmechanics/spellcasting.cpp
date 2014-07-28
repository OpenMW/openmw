#include "spellcasting.hpp"

#include <cfloat>

#include <boost/format.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/animation.hpp"

#include "magiceffects.hpp"
#include "npcstats.hpp"

namespace
{

    /// Get projectile properties (model, sound and speed) for a spell with the given effects
    /// If \a model is empty, the spell has no ranged effects and should not spawn a projectile.
    void getProjectileInfo (const ESM::EffectList& effects, std::string& model, std::string& sound, float& speed)
    {
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.mList.begin());
            iter!=effects.mList.end(); ++iter)
        {
            if (iter->mRange != ESM::RT_Target)
                continue;

            const ESM::MagicEffect *magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                iter->mEffectID);

            model = magicEffect->mBolt;
            if (model.empty())
                model = "VFX_DefaultBolt";

            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };
            if (!magicEffect->mBoltSound.empty())
                sound = magicEffect->mBoltSound;
            else
                sound = schools[magicEffect->mData.mSchool] + " bolt";

            speed = magicEffect->mData.mSpeed;
            break;
        }
    }

}

namespace MWMechanics
{

    ESM::Skill::SkillEnum spellSchoolToSkill(int school)
    {
        std::map<int, ESM::Skill::SkillEnum> schoolSkillMap; // maps spell school to skill id
        schoolSkillMap[0] = ESM::Skill::Alteration;
        schoolSkillMap[1] = ESM::Skill::Conjuration;
        schoolSkillMap[3] = ESM::Skill::Illusion;
        schoolSkillMap[2] = ESM::Skill::Destruction;
        schoolSkillMap[4] = ESM::Skill::Mysticism;
        schoolSkillMap[5] = ESM::Skill::Restoration;
        assert(schoolSkillMap.find(school) != schoolSkillMap.end());
        return schoolSkillMap[school];
    }

    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool)
    {
        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).mMagnitude)
            return 0;

        float y = FLT_MAX;
        float lowestSkill = 0;

        for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != spell->mEffects.mList.end(); ++it)
        {
            float x = it->mDuration;
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                        it->mEffectID);
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage))
                x = std::max(1.f, x);
            x *= 0.1 * magicEffect->mData.mBaseCost;
            x *= 0.5 * (it->mMagnMin + it->mMagnMax);
            x *= it->mArea * 0.05 * magicEffect->mData.mBaseCost;
            if (it->mRange == ESM::RT_Target)
                x *= 1.5;
            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fEffectCostMult")->getFloat();
            x *= fEffectCostMult;

            float s = 2 * actor.getClass().getSkill(actor, spellSchoolToSkill(magicEffect->mData.mSchool));
            if (s - x < y)
            {
                y = s - x;
                if (effectiveSchool)
                    *effectiveSchool = magicEffect->mData.mSchool;
                lowestSkill = s;
            }
        }

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100;

        int castBonus = -stats.getMagicEffects().get(ESM::MagicEffect::Sound).mMagnitude;

        int actorWillpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        int actorLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();

        float castChance = (lowestSkill - spell->mData.mCost + castBonus + 0.2 * actorWillpower + 0.1 * actorLuck) * stats.getFatigueTerm();
        if (MWBase::Environment::get().getWorld()->getGodModeState() && actor.getRefData().getHandle() == "player")
            castChance = 100;

        return std::max(0.f, std::min(100.f, castChance));
    }

    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        return getSpellSuccessChance(spell, actor, effectiveSchool);
    }


    int getSpellSchool(const std::string& spellId, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spellId, actor, &school);
        return school;
    }

    int getSpellSchool(const ESM::Spell* spell, const MWWorld::Ptr& actor)
    {
        int school = 0;
        getSpellSuccessChance(spell, actor, &school);
        return school;
    }

    float getEffectResistanceAttribute (short effectId, const MagicEffects* actorEffects)
    {
        short resistanceEffect = ESM::MagicEffect::getResistanceEffect(effectId);
        short weaknessEffect = ESM::MagicEffect::getWeaknessEffect(effectId);

        float resistance = 0;
        if (resistanceEffect != -1)
            resistance += actorEffects->get(resistanceEffect).mMagnitude;
        if (weaknessEffect != -1)
            resistance -= actorEffects->get(weaknessEffect).mMagnitude;

        if (effectId == ESM::MagicEffect::FireDamage)
            resistance += actorEffects->get(ESM::MagicEffect::FireShield).mMagnitude;
        if (effectId == ESM::MagicEffect::ShockDamage)
            resistance += actorEffects->get(ESM::MagicEffect::LightningShield).mMagnitude;
        if (effectId == ESM::MagicEffect::FrostDamage)
            resistance += actorEffects->get(ESM::MagicEffect::FrostShield).mMagnitude;

        return resistance;
    }

    float getEffectResistance (short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                               const ESM::Spell* spell, const MagicEffects* effects)
    {
        const ESM::MagicEffect *magicEffect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
            effectId);

        const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWMechanics::MagicEffects* magicEffects = &stats.getMagicEffects();
        if (effects)
            magicEffects = effects;

        float resisted = 0;
        if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
        {
            float resistance = getEffectResistanceAttribute(effectId, magicEffects);

            float willpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
            float luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
            float x = (willpower + 0.1 * luck) * stats.getFatigueTerm();

            // This makes spells that are easy to cast harder to resist and vice versa
            if (spell != NULL && !caster.isEmpty() && caster.getClass().isActor())
            {
                float castChance = getSpellSuccessChance(spell, caster);
                if (castChance > 0)
                    x *= 50 / castChance;
            }

            float roll = static_cast<float>(std::rand()) / RAND_MAX * 100;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude)
                roll -= resistance;

            if (x <= roll)
                x = 0;
            else
            {
                if (magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude)
                    x = 100;
                else
                    x = roll / std::min(x, 100.f);
            }

            x = std::min(x + resistance, 100.f);

            resisted = x;
        }

        return resisted;
    }

    float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                              const ESM::Spell* spell, const MagicEffects* effects)
    {
        float resistance = getEffectResistance(effectId, actor, caster, spell, effects);
        if (resistance >= 0)
            return 1 - resistance / 100.f;
        else
            return -(resistance-100) / 100.f;
    }

    CastSpell::CastSpell(const MWWorld::Ptr &caster, const MWWorld::Ptr &target)
        : mCaster(caster)
        , mTarget(target)
        , mStack(false)
        , mHitPosition(0,0,0)
    {
    }

    void CastSpell::inflict(const MWWorld::Ptr &target, const MWWorld::Ptr &caster,
                            const ESM::EffectList &effects, ESM::RangeType range, bool reflected, bool exploded)
    {
        if (target.getClass().isActor() && target.getClass().getCreatureStats(target).isDead())
            return;

        // If none of the effects need to apply, we can early-out
        bool found = false;
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.mList.begin());
            iter!=effects.mList.end(); ++iter)
        {
            if (iter->mRange != range)
                continue;
            found = true;
        }
        if (!found)
            return;

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (mId);
        if (spell && (spell->mData.mType == ESM::Spell::ST_Disease || spell->mData.mType == ESM::Spell::ST_Blight))
        {
            float x = (spell->mData.mType == ESM::Spell::ST_Disease) ?
                        target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::ResistCommonDisease).mMagnitude
                      : target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::ResistBlightDisease).mMagnitude;

            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            if (roll <= x)
            {
                // Fully resisted, show message
                if (target.getRefData().getHandle() == "player")
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                return;
            }
        }

        ESM::EffectList reflectedEffects;
        std::vector<ActiveSpells::ActiveEffect> appliedLastingEffects;
        bool firstAppliedEffect = true;
        bool anyHarmfulEffect = false;

        // HACK: cache target's magic effects here, and add any applied effects to it. Use the cached effects for determining resistance.
        // This is required for Weakness effects in a spell to apply to any subsequent effects in the spell.
        // Otherwise, they'd only apply after the whole spell was added.
        MagicEffects targetEffects;
        if (target.getClass().isActor())
            targetEffects += target.getClass().getCreatureStats(target).getMagicEffects();

        bool castByPlayer = (!caster.isEmpty() && caster.getRefData().getHandle() == "player");

        // Try absorbing if it's a spell
        // NOTE: Vanilla does this once per effect source instead of adding the % from all sources together, not sure
        // if that is worth replicating.
        bool absorbed = false;
        if (spell && caster != target && target.getClass().isActor())
        {
            int absorb = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).mMagnitude;
            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            absorbed = (roll < absorb);
            if (absorbed)
            {
                const ESM::Static* absorbStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Absorb");
                MWBase::Environment::get().getWorld()->getAnimation(target)->addEffect(
                            "meshes\\" + absorbStatic->mModel, ESM::MagicEffect::SpellAbsorption, false, "");
                // Magicka is increased by cost of spell
                DynamicStat<float> magicka = target.getClass().getCreatureStats(target).getMagicka();
                magicka.setCurrent(magicka.getCurrent() + spell->mData.mCost);
                target.getClass().getCreatureStats(target).setMagicka(magicka);
            }
        }

        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (effects.mList.begin());
            effectIt!=effects.mList.end(); ++effectIt)
        {
            if (effectIt->mRange != range)
                continue;

            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                effectIt->mEffectID);

            if (!MWBase::Environment::get().getWorld()->isLevitationEnabled() && effectIt->mEffectID == ESM::MagicEffect::Levitate)
            {
                if (castByPlayer)
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sLevitateDisabled}");
                continue;
            }

            if (!MWBase::Environment::get().getWorld()->isTeleportingEnabled() &&
                (effectIt->mEffectID == ESM::MagicEffect::AlmsiviIntervention ||
                 effectIt->mEffectID == ESM::MagicEffect::DivineIntervention ||
                 effectIt->mEffectID == ESM::MagicEffect::Mark ||
                 effectIt->mEffectID == ESM::MagicEffect::Recall))
            {
                if (castByPlayer)
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                continue;
            }

            // If player is healing someone, show the target's HP bar
            if (castByPlayer && target != caster
                    && effectIt->mEffectID == ESM::MagicEffect::RestoreHealth
                    && target.getClass().isActor())
                MWBase::Environment::get().getWindowManager()->setEnemy(target);

            float magnitudeMult = 1;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful && target.getClass().isActor())
            {
                anyHarmfulEffect = true;

                if (absorbed) // Absorbed, and we know there was a harmful effect (figuring that out is the only reason we are in this loop)
                    break;

                // If player is attempting to cast a harmful spell, show the target's HP bar
                if (castByPlayer && target != caster)
                    MWBase::Environment::get().getWindowManager()->setEnemy(target);

                // Try reflecting
                if (!reflected && magnitudeMult > 0 && !caster.isEmpty() && caster != target && !(magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable))
                {
                    int reflect = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Reflect).mMagnitude;
                    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                    bool isReflected = (roll < reflect);
                    if (isReflected)
                    {
                        const ESM::Static* reflectStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Reflect");
                        MWBase::Environment::get().getWorld()->getAnimation(target)->addEffect(
                                    "meshes\\" + reflectStatic->mModel, ESM::MagicEffect::Reflect, false, "");
                        reflectedEffects.mList.push_back(*effectIt);
                        magnitudeMult = 0;
                    }
                }

                // Try resisting
                if (magnitudeMult > 0 && target.getClass().isActor())
                {
                    magnitudeMult = MWMechanics::getEffectMultiplier(effectIt->mEffectID, target, caster, spell, &targetEffects);
                    if (magnitudeMult == 0)
                    {
                        // Fully resisted, show message
                        if (target.getRefData().getHandle() == "player")
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                        else if (castByPlayer)
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResisted}");
                    }
                }
            }

            if (magnitudeMult > 0 && !absorbed)
            {
                float random = std::rand() / static_cast<float>(RAND_MAX);
                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                magnitude *= magnitudeMult;

                bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
                if (target.getClass().isActor() && hasDuration)
                {
                    ActiveSpells::ActiveEffect effect;
                    effect.mEffectId = effectIt->mEffectID;
                    effect.mArg = MWMechanics::EffectKey(*effectIt).mArg;
                    effect.mDuration = effectIt->mDuration;
                    effect.mMagnitude = magnitude;

                    targetEffects.add(MWMechanics::EffectKey(*effectIt), MWMechanics::EffectParam(effect.mMagnitude));

                    appliedLastingEffects.push_back(effect);

                    // For absorb effects, also apply the effect to the caster - but with a negative
                    // magnitude, since we're transfering stats from the target to the caster
                    if (!caster.isEmpty() && caster.getClass().isActor())
                    {
                        for (int i=0; i<5; ++i)
                        {
                            if (effectIt->mEffectID == ESM::MagicEffect::AbsorbAttribute+i)
                            {
                                std::vector<ActiveSpells::ActiveEffect> effects;
                                ActiveSpells::ActiveEffect effect_ = effect;
                                effect_.mMagnitude *= -1;
                                effects.push_back(effect_);
                                // Also make sure to set casterActorId = target, so that the effect on the caster gets purged when the target dies
                                caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell("", true,
                                            effects, mSourceName, target.getClass().getCreatureStats(target).getActorId());
                            }
                        }
                    }
                }
                else
                    applyInstantEffect(target, caster, EffectKey(*effectIt), magnitude);

                // HACK: Damage attribute/skill actually has a duration, even though the actual effect is instant and permanent.
                // This was probably just done to have the effect visible in the magic menu for a while
                // to notify the player they've been damaged?
                if (effectIt->mEffectID == ESM::MagicEffect::DamageAttribute
                        || effectIt->mEffectID == ESM::MagicEffect::DamageSkill
                        || effectIt->mEffectID == ESM::MagicEffect::RestoreAttribute
                        || effectIt->mEffectID == ESM::MagicEffect::RestoreSkill
                        )
                    applyInstantEffect(target, caster, EffectKey(*effectIt), magnitude);

                if (target.getClass().isActor() || magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
                {
                    // Play sound, only for the first effect
                    if (firstAppliedEffect)
                    {
                        static const std::string schools[] = {
                            "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
                        };

                        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                        if(!magicEffect->mHitSound.empty())
                            sndMgr->playSound3D(target, magicEffect->mHitSound, 1.0f, 1.0f);
                        else
                            sndMgr->playSound3D(target, schools[magicEffect->mData.mSchool]+" hit", 1.0f, 1.0f);
                        firstAppliedEffect = false;
                    }

                    // Add VFX
                    const ESM::Static* castStatic;
                    if (!magicEffect->mHit.empty())
                        castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
                    else
                        castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_DefaultHit");

                    // TODO: VFX are no longer active after saving/reloading the game
                    bool loop = magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx;
                    // Note: in case of non actor, a free effect should be fine as well
                    MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
                    if (anim)
                        anim->addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "");
                }
            }
        }

        if (!exploded)
            MWBase::Environment::get().getWorld()->explodeSpell(mHitPosition, effects, caster, mId, mSourceName);

        if (!reflectedEffects.mList.empty())
            inflict(caster, target, reflectedEffects, range, true);

        if (!appliedLastingEffects.empty())
        {
            int casterActorId = -1;
            if (!caster.isEmpty() && caster.getClass().isActor())
                casterActorId = caster.getClass().getCreatureStats(caster).getActorId();
            target.getClass().getCreatureStats(target).getActiveSpells().addSpell(mId, mStack, appliedLastingEffects,
                                                                                  mSourceName, casterActorId);
        }

        // Notify the target actor they've been hit
        if (anyHarmfulEffect && target.getClass().isActor() && target != caster && !caster.isEmpty() && caster.getClass().isActor())
            target.getClass().onHit(target, 0.f, true, MWWorld::Ptr(), caster, true);
    }

    void CastSpell::applyInstantEffect(const MWWorld::Ptr &target, const MWWorld::Ptr &caster, const MWMechanics::EffectKey& effect, float magnitude)
    {
        short effectId = effect.mId;
        if (!target.getClass().isActor())
        {
            if (effectId == ESM::MagicEffect::Lock)
            {
                if (target.getCellRef().getLockLevel() < magnitude) //If the door is not already locked to a higher value, lock it to spell magnitude
                {
                    if (caster.getRefData().getHandle() == "player")
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicLockSuccess}");
                    target.getCellRef().setLockLevel(magnitude);
                }
            }
            else if (effectId == ESM::MagicEffect::Open)
            {
                if (target.getCellRef().getLockLevel() <= magnitude)
                {
                    if (target.getCellRef().getLockLevel() > 0)
                    {
                        MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock", 1.f, 1.f);
                        if (!caster.isEmpty() && caster.getClass().isActor())
                            MWBase::Environment::get().getMechanicsManager()->objectOpened(caster, target);

                        if (caster.getRefData().getHandle() == "player")
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicOpenSuccess}");
                    }
                    target.getCellRef().setLockLevel(-abs(target.getCellRef().getLockLevel()));
                }
                else
                    MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock Fail", 1.f, 1.f);
            }
        }
        else
        {
            if (effectId == ESM::MagicEffect::DamageAttribute || effectId == ESM::MagicEffect::RestoreAttribute)
            {
                int attribute = effect.mArg;
                AttributeValue value = target.getClass().getCreatureStats(target).getAttribute(attribute);
                if (effectId == ESM::MagicEffect::DamageAttribute)
                    value.damage(magnitude);
                else
                    value.restore(magnitude);
                target.getClass().getCreatureStats(target).setAttribute(attribute, value);
            }
            else if (effectId == ESM::MagicEffect::DamageSkill || effectId == ESM::MagicEffect::RestoreSkill)
            {
                if (target.getTypeName() != typeid(ESM::NPC).name())
                    return;
                int skill = effect.mArg;
                SkillValue& value = target.getClass().getNpcStats(target).getSkill(skill);
                if (effectId == ESM::MagicEffect::DamageSkill)
                    value.damage(magnitude);
                else
                    value.restore(magnitude);
            }

            if (effectId == ESM::MagicEffect::CurePoison)
                target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(ESM::MagicEffect::Poison);
            else if (effectId == ESM::MagicEffect::CureParalyzation)
                target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(ESM::MagicEffect::Paralyze);
            else if (effectId == ESM::MagicEffect::CureCommonDisease)
                target.getClass().getCreatureStats(target).getSpells().purgeCommonDisease();
            else if (effectId == ESM::MagicEffect::CureBlightDisease)
                target.getClass().getCreatureStats(target).getSpells().purgeBlightDisease();
            else if (effectId == ESM::MagicEffect::CureCorprusDisease)
                target.getClass().getCreatureStats(target).getSpells().purgeCorprusDisease();
            else if (effectId == ESM::MagicEffect::Dispel)
                target.getClass().getCreatureStats(target).getActiveSpells().purgeAll(magnitude);
            else if (effectId == ESM::MagicEffect::RemoveCurse)
                target.getClass().getCreatureStats(target).getSpells().purgeCurses();

            if (target.getRefData().getHandle() != "player")
                return;
            if (!MWBase::Environment::get().getWorld()->isTeleportingEnabled())
                return;

            if (effectId == ESM::MagicEffect::DivineIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "divinemarker");
            }
            else if (effectId == ESM::MagicEffect::AlmsiviIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "templemarker");
            }

            else if (effectId == ESM::MagicEffect::Mark)
            {
                MWBase::Environment::get().getWorld()->getPlayer().markPosition(
                            target.getCell(), target.getRefData().getPosition());
            }
            else if (effectId == ESM::MagicEffect::Recall)
            {
                MWWorld::CellStore* markedCell = NULL;
                ESM::Position markedPosition;

                MWBase::Environment::get().getWorld()->getPlayer().getMarkedPosition(markedCell, markedPosition);
                if (markedCell)
                {
                    MWWorld::ActionTeleport action(markedCell->isExterior() ? "" : markedCell->getCell()->mName,
                                            markedPosition);
                    action.execute(target);
                }
            }
        }
    }

    bool CastSpell::cast(const std::string &id)
    {
        if (const ESM::Spell *spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (id))
            return cast(spell);

        if (const ESM::Potion *potion =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>().search (id))
            return cast(potion);

        if (const ESM::Ingredient *ingredient =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Ingredient>().search (id))
            return cast(ingredient);

        throw std::runtime_error("ID type cannot be casted");
    }

    bool CastSpell::cast(const MWWorld::Ptr &item)
    {
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            throw std::runtime_error("can't cast an item without an enchantment");

        mSourceName = item.getClass().getName(item);
        mId = item.getCellRef().getRefId();

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);

        mStack = (enchantment->mData.mType == ESM::Enchantment::CastOnce);

        // Check if there's enough charge left
        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed || enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
        {
            const float enchantCost = enchantment->mData.mCost;
            int eSkill = mCaster.getClass().getSkill(mCaster, ESM::Skill::Enchant);
            const int castCost = std::max(1.f, enchantCost - (enchantCost / 100) * (eSkill - 10));

            if (item.getCellRef().getEnchantmentCharge() == -1)
                item.getCellRef().setEnchantmentCharge(enchantment->mData.mCharge);

            if (item.getCellRef().getEnchantmentCharge() < castCost)
            {
                // TODO: Should there be a sound here?
                if (mCaster.getRefData().getHandle() == "player")
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInsufficientCharge}");
                return false;
            }
            // Reduce charge
            item.getCellRef().setEnchantmentCharge(item.getCellRef().getEnchantmentCharge() - castCost);
        }

        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed)
        {
            if (mCaster.getRefData().getHandle() == "player")
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 1);
        }
        if (enchantment->mData.mType == ESM::Enchantment::CastOnce)
            item.getContainerStore()->remove(item, 1, mCaster);
        else if (enchantment->mData.mType != ESM::Enchantment::WhenStrikes)
        {
            if (mCaster.getRefData().getHandle() == "player")
            {
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 3);
            }
        }

        inflict(mCaster, mCaster, enchantment->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
        {
            if (!mTarget.getClass().isActor() || !mTarget.getClass().getCreatureStats(mTarget).isDead())
                inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Touch);
        }

        std::string projectileModel;
        std::string sound;
        float speed = 0;
        getProjectileInfo(enchantment->mEffects, projectileModel, sound, speed);
        if (!projectileModel.empty())
            MWBase::Environment::get().getWorld()->launchMagicBolt(projectileModel, sound, mId, speed,
                                                               false, enchantment->mEffects, mCaster, mSourceName,
                                                                   // Not needed, enchantments can only be cast by actors
                                                                   Ogre::Vector3(1,0,0));

        return true;
    }

    bool CastSpell::cast(const ESM::Potion* potion)
    {
        mSourceName = potion->mName;
        mId = potion->mId;
        mStack = true;

        inflict(mCaster, mCaster, potion->mEffects, ESM::RT_Self);

        return true;
    }

    bool CastSpell::cast(const ESM::Spell* spell)
    {
        mSourceName = spell->mName;
        mId = spell->mId;
        mStack = false;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        int school = 0;

        if (mCaster.getClass().isActor())
        {
            school = getSpellSchool(spell, mCaster);

            CreatureStats& stats = mCaster.getClass().getCreatureStats(mCaster);

            // Reduce fatigue (note that in the vanilla game, both GMSTs are 0, and there's no fatigue loss)
            static const float fFatigueSpellBase = store.get<ESM::GameSetting>().find("fFatigueSpellBase")->getFloat();
            static const float fFatigueSpellMult = store.get<ESM::GameSetting>().find("fFatigueSpellMult")->getFloat();
            DynamicStat<float> fatigue = stats.getFatigue();
            const float normalizedEncumbrance = mCaster.getClass().getEncumbrance(mCaster) / mCaster.getClass().getCapacity(mCaster);
            float fatigueLoss = spell->mData.mCost * (fFatigueSpellBase + normalizedEncumbrance * fFatigueSpellMult);
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss); stats.setFatigue(fatigue);

            bool fail = false;

            // Check success
            int successChance = getSpellSuccessChance(spell, mCaster);
            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            if (!fail && roll >= successChance)
            {
                if (mCaster.getRefData().getHandle() == "player")
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicSkillFail}");
                fail = true;
            }

            if (fail)
            {
                // Failure sound
                static const std::string schools[] = {
                    "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
                };

                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                sndMgr->playSound3D(mCaster, "Spell Failure " + schools[school], 1.0f, 1.0f);
                return false;
            }
        }

        if (mCaster.getRefData().getHandle() == "player" && spell->mData.mType == ESM::Spell::ST_Spell)
            mCaster.getClass().skillUsageSucceeded(mCaster,
                spellSchoolToSkill(school), 0);

        inflict(mCaster, mCaster, spell->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
        {
            if (!mTarget.getClass().isActor() || !mTarget.getClass().getCreatureStats(mTarget).isDead())
            {
                inflict(mTarget, mCaster, spell->mEffects, ESM::RT_Touch);
            }
        }


        std::string projectileModel;
        std::string sound;
        float speed = 0;
        getProjectileInfo(spell->mEffects, projectileModel, sound, speed);
        if (!projectileModel.empty())
        {
            Ogre::Vector3 fallbackDirection (0,1,0);
            // Fall back to a "caster to target" direction if we have no other means of determining it
            // (e.g. when cast by a non-actor)
            if (!mTarget.isEmpty())
                fallbackDirection =
                   Ogre::Vector3(mTarget.getRefData().getPosition().pos)-
                   Ogre::Vector3(mCaster.getRefData().getPosition().pos);

            MWBase::Environment::get().getWorld()->launchMagicBolt(projectileModel, sound, mId, speed,
                       false, spell->mEffects, mCaster, mSourceName, fallbackDirection);
        }

        return true;
    }

    bool CastSpell::cast (const ESM::Ingredient* ingredient)
    {
        mId = ingredient->mId;
        mStack = true;
        mSourceName = ingredient->mName;

        ESM::ENAMstruct effect;
        effect.mEffectID = ingredient->mData.mEffectID[0];
        effect.mSkill = ingredient->mData.mSkills[0];
        effect.mAttribute = ingredient->mData.mAttributes[0];
        effect.mRange = ESM::RT_Self;
        effect.mArea = 0;

        const ESM::MagicEffect *magicEffect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
            effect.mEffectID);

        const MWMechanics::NpcStats& npcStats = mCaster.getClass().getNpcStats(mCaster);
        const MWMechanics::CreatureStats& creatureStats = mCaster.getClass().getCreatureStats(mCaster);

        float x = (npcStats.getSkill (ESM::Skill::Alchemy).getModified() +
                    0.2 * creatureStats.getAttribute (ESM::Attribute::Intelligence).getModified()
                    + 0.1 * creatureStats.getAttribute (ESM::Attribute::Luck).getModified())
                    * creatureStats.getFatigueTerm();

        int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
        if (roll > x)
        {
            // "X has no effect on you"
            std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage50")->getString();
            message = boost::str(boost::format(message) % ingredient->mName);
            MWBase::Environment::get().getWindowManager()->messageBox(message);
            return false;
        }

        float magnitude = 0;
        float y = roll / std::min(x, 100.f);
        y *= 0.25 * x;
        if (magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
            effect.mDuration = int(y);
        else
            effect.mDuration = 1;
        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
        {
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
                magnitude = int((0.05 * y) / (0.1 * magicEffect->mData.mBaseCost));
            else
                magnitude = int(y / (0.1 * magicEffect->mData.mBaseCost));
            magnitude = std::max(1.f, magnitude);
        }
        else
            magnitude = 1;

        effect.mMagnMax = magnitude;
        effect.mMagnMin = magnitude;

        ESM::EffectList effects;
        effects.mList.push_back(effect);

        inflict(mCaster, mCaster, effects, ESM::RT_Self);

        return true;
    }

}
