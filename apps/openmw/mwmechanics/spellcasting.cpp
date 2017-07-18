#include "spellcasting.hpp"

#include <cfloat>
#include <limits>
#include <iomanip>

#include <boost/format.hpp>

#include <components/misc/rng.hpp>

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
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/animation.hpp"

#include "magiceffects.hpp"
#include "npcstats.hpp"
#include "actorutil.hpp"
#include "aifollow.hpp"

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

    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap)
    {
        float y = std::numeric_limits<float>::max();
        float lowestSkill = 0;

        for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != spell->mEffects.mList.end(); ++it)
        {
            float x = static_cast<float>(it->mDuration);
            const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                        it->mEffectID);
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::UncappedDamage))
                x = std::max(1.f, x);
            x *= 0.1f * magicEffect->mData.mBaseCost;
            x *= 0.5f * (it->mMagnMin + it->mMagnMax);
            x *= it->mArea * 0.05f * magicEffect->mData.mBaseCost;
            if (it->mRange == ESM::RT_Target)
                x *= 1.5f;
            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fEffectCostMult")->getFloat();
            x *= fEffectCostMult;

            float s = 2.0f * actor.getClass().getSkill(actor, spellSchoolToSkill(magicEffect->mData.mSchool));
            if (s - x < y)
            {
                y = s - x;
                if (effectiveSchool)
                    *effectiveSchool = magicEffect->mData.mSchool;
                lowestSkill = s;
            }
        }

        bool godmode = actor == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).getMagnitude()&& !godmode)
            return 0;

        if (spell->mData.mType == ESM::Spell::ST_Power)
            return stats.getSpells().canUsePower(spell) ? 100 : 0;

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100;

        if (godmode)
        {
            return 100;
        }

        float castBonus = -stats.getMagicEffects().get(ESM::MagicEffect::Sound).getMagnitude();

        int actorWillpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        int actorLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();

        float castChance = (lowestSkill - spell->mData.mCost + castBonus + 0.2f * actorWillpower + 0.1f * actorLuck) * stats.getFatigueTerm();

        if (!cap)
            return std::max(0.f, castChance);
        else
            return std::max(0.f, std::min(100.f, castChance));
    }

    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        return getSpellSuccessChance(spell, actor, effectiveSchool, cap);
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

    bool spellIncreasesSkill(const ESM::Spell *spell)
    {
        if (spell->mData.mType == ESM::Spell::ST_Spell && !(spell->mData.mFlags & ESM::Spell::F_Always))
            return true;
        return false;
    }

    bool spellIncreasesSkill(const std::string &spellId)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);
        return spellIncreasesSkill(spell);
    }

    float getEffectResistanceAttribute (short effectId, const MagicEffects* actorEffects)
    {
        short resistanceEffect = ESM::MagicEffect::getResistanceEffect(effectId);
        short weaknessEffect = ESM::MagicEffect::getWeaknessEffect(effectId);

        float resistance = 0;
        if (resistanceEffect != -1)
            resistance += actorEffects->get(resistanceEffect).getMagnitude();
        if (weaknessEffect != -1)
            resistance -= actorEffects->get(weaknessEffect).getMagnitude();

        if (effectId == ESM::MagicEffect::FireDamage)
            resistance += actorEffects->get(ESM::MagicEffect::FireShield).getMagnitude();
        if (effectId == ESM::MagicEffect::ShockDamage)
            resistance += actorEffects->get(ESM::MagicEffect::LightningShield).getMagnitude();
        if (effectId == ESM::MagicEffect::FrostDamage)
            resistance += actorEffects->get(ESM::MagicEffect::FrostShield).getMagnitude();

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
            // Effects with no resistance attribute belonging to them can not be resisted
            if (ESM::MagicEffect::getResistanceEffect(effectId) == -1)
                return 0.f;

            float resistance = getEffectResistanceAttribute(effectId, magicEffects);

            int willpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
            float luck = static_cast<float>(stats.getAttribute(ESM::Attribute::Luck).getModified());
            float x = (willpower + 0.1f * luck) * stats.getFatigueTerm();

            // This makes spells that are easy to cast harder to resist and vice versa
            float castChance = 100.f;
            if (spell != NULL && !caster.isEmpty() && caster.getClass().isActor())
            {
                castChance = getSpellSuccessChance(spell, caster, NULL, false); // Uncapped casting chance
            }
            if (castChance > 0)
                x *= 50 / castChance;

            float roll = Misc::Rng::rollClosedProbability() * 100;
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
        return 1 - resistance / 100.f;
    }

    /// Check if the given effect can be applied to the target. If \a castByPlayer, emits a message box on failure.
    bool checkEffectTarget (int effectId, const MWWorld::Ptr& target, const MWWorld::Ptr& caster, bool castByPlayer)
    {
        switch (effectId)
        {
            case ESM::MagicEffect::Levitate:
                if (!MWBase::Environment::get().getWorld()->isLevitationEnabled())
                {
                    if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sLevitateDisabled}");
                    return false;
                }
                break;
            case ESM::MagicEffect::Soultrap:
                if (!target.getClass().isNpc() // no messagebox for NPCs
                     && (target.getTypeName() == typeid(ESM::Creature).name() && target.get<ESM::Creature>()->mBase->mData.mSoul == 0))
                {
                    if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInvalidTarget}");
                    return true; // must still apply to get visual effect and have target regard it as attack
                }
                break;
            case ESM::MagicEffect::AlmsiviIntervention:
            case ESM::MagicEffect::DivineIntervention:
            case ESM::MagicEffect::Mark:
            case ESM::MagicEffect::Recall:
                if (!MWBase::Environment::get().getWorld()->isTeleportingEnabled())
                {
                    if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                    return false;
                }
                break;
            case ESM::MagicEffect::WaterWalking:
                if (target.getClass().isPureWaterCreature(target) && MWBase::Environment::get().getWorld()->isSwimming(target))
                    return false;

                MWBase::World *world = MWBase::Environment::get().getWorld();

                if (!world->isWaterWalkingCastableOnTarget(target))
                {
                    if (castByPlayer && caster == target)
                        MWBase::Environment::get().getWindowManager()->messageBox ("#{sMagicInvalidEffect}");
                    return false;
                }
                break;
        }
        return true;
    }

    CastSpell::CastSpell(const MWWorld::Ptr &caster, const MWWorld::Ptr &target, const bool fromProjectile)
        : mCaster(caster)
        , mTarget(target)
        , mStack(false)
        , mHitPosition(0,0,0)
        , mAlwaysSucceed(false)
        , mFromProjectile(fromProjectile)
    {
    }

    void CastSpell::launchMagicBolt (const ESM::EffectList& effects)
    {        
        osg::Vec3f fallbackDirection (0,1,0);     

        // Fall back to a "caster to target" direction if we have no other means of determining it
        // (e.g. when cast by a non-actor)
        if (!mTarget.isEmpty())
            fallbackDirection =
                osg::Vec3f(mTarget.getRefData().getPosition().asVec3())-
                osg::Vec3f(mCaster.getRefData().getPosition().asVec3());
            
        MWBase::Environment::get().getWorld()->launchMagicBolt(mId, false, effects,
                                                   mCaster, mSourceName, fallbackDirection);
    }

    void CastSpell::inflict(const MWWorld::Ptr &target, const MWWorld::Ptr &caster,
                            const ESM::EffectList &effects, ESM::RangeType range, bool reflected, bool exploded)
    {
        if (!target.isEmpty() && target.getClass().isActor() && target.getClass().getCreatureStats(target).isDead())
            return;

        // If none of the effects need to apply, we can early-out
        bool found = false;
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (effects.mList.begin());
            iter!=effects.mList.end(); ++iter)
        {
            if (iter->mRange == range)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return;

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (mId);
        if (spell && !target.isEmpty() && (spell->mData.mType == ESM::Spell::ST_Disease || spell->mData.mType == ESM::Spell::ST_Blight))
        {
            int requiredResistance = (spell->mData.mType == ESM::Spell::ST_Disease) ?
                ESM::MagicEffect::ResistCommonDisease
                : ESM::MagicEffect::ResistBlightDisease;
            float x = target.getClass().getCreatureStats(target).getMagicEffects().get(requiredResistance).getMagnitude();

            if (Misc::Rng::roll0to99() <= x)
            {
                // Fully resisted, show message
                if (target == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                return;
            }
        }

        ESM::EffectList reflectedEffects;
        std::vector<ActiveSpells::ActiveEffect> appliedLastingEffects;

        // HACK: cache target's magic effects here, and add any applied effects to it. Use the cached effects for determining resistance.
        // This is required for Weakness effects in a spell to apply to any subsequent effects in the spell.
        // Otherwise, they'd only apply after the whole spell was added.
        MagicEffects targetEffects;
        if (!target.isEmpty() && target.getClass().isActor())
            targetEffects += target.getClass().getCreatureStats(target).getMagicEffects();

        bool castByPlayer = (!caster.isEmpty() && caster == getPlayer());

        ActiveSpells targetSpells;
        if (!target.isEmpty() && target.getClass().isActor())
            targetSpells = target.getClass().getCreatureStats(target).getActiveSpells();

        bool canCastAnEffect = false;    // For bound equipment.If this remains false
                                         // throughout the iteration of this spell's 
                                         // effects, we display a "can't re-cast" message

        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (effects.mList.begin());
             !target.isEmpty() && effectIt != effects.mList.end(); ++effectIt)
        {
            if (effectIt->mRange != range)
                continue;

            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                effectIt->mEffectID);

            // Re-casting a bound equipment effect has no effect if the spell is still active
            if (magicEffect->mData.mFlags & ESM::MagicEffect::NonRecastable && targetSpells.isSpellActive(mId))
            {
                if (effectIt == (effects.mList.end() - 1) && !canCastAnEffect && castByPlayer)
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicCannotRecast}");
                continue;
            }
            else
                canCastAnEffect = true;

            if (!checkEffectTarget(effectIt->mEffectID, target, caster, castByPlayer))
                continue;

            // caster needs to be an actor for linked effects (e.g. Absorb)
            if (magicEffect->mData.mFlags & ESM::MagicEffect::CasterLinked
                    && (caster.isEmpty() || !caster.getClass().isActor()))
                continue;

            // If player is healing someone, show the target's HP bar
            if (castByPlayer && target != caster
                    && effectIt->mEffectID == ESM::MagicEffect::RestoreHealth
                    && target.getClass().isActor())
                MWBase::Environment::get().getWindowManager()->setEnemy(target);

            // Try absorbing if it's a spell
            // NOTE: Vanilla does this once per spell absorption effect source instead of adding the % from all sources together, not sure
            // if that is worth replicating.
            bool absorbed = false;
            if (spell && caster != target && target.getClass().isActor())
            {
                float absorb = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).getMagnitude();
                absorbed = (Misc::Rng::roll0to99() < absorb);
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

            float magnitudeMult = 1;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful && target.getClass().isActor())
            {
                if (absorbed)
                    continue;

                // Try reflecting
                if (!reflected && !caster.isEmpty() && caster != target && !(magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable))
                {
                    float reflect = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Reflect).getMagnitude();
                    bool isReflected = (Misc::Rng::roll0to99() < reflect);
                    if (isReflected)
                    {
                        const ESM::Static* reflectStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Reflect");
                        MWBase::Environment::get().getWorld()->getAnimation(target)->addEffect(
                                    "meshes\\" + reflectStatic->mModel, ESM::MagicEffect::Reflect, false, "");
                        reflectedEffects.mList.push_back(*effectIt);
                        continue;
                    }
                }

                // Try resisting
                magnitudeMult = MWMechanics::getEffectMultiplier(effectIt->mEffectID, target, caster, spell, &targetEffects);
                if (magnitudeMult == 0)
                {
                    // Fully resisted, show message
                    if (target == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                    else if (castByPlayer)
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResisted}");
                }

                // If player is attempting to cast a harmful spell, show the target's HP bar
                if (castByPlayer && target != caster)
                    MWBase::Environment::get().getWindowManager()->setEnemy(target);

                // Notify the target actor they've been hit
                if (target != caster && !caster.isEmpty())
                    target.getClass().onHit(target, 0.0f, true, MWWorld::Ptr(), caster, osg::Vec3f(), true);
            }

            if (magnitudeMult > 0 && !absorbed)
            {
                float random = Misc::Rng::rollClosedProbability();
                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                magnitude *= magnitudeMult;

                if (!target.getClass().isActor())
                {
                    // non-actor objects have no list of active magic effects, so have to apply instantly
                    if (!applyInstantEffect(target, caster, EffectKey(*effectIt), magnitude))
                        continue;
                }
                else // target.getClass().isActor() == true
                {
                    bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
                    if (hasDuration && effectIt->mDuration == 0)
                    {
                        // duration 0 means apply full magnitude instantly
                        bool wasDead = target.getClass().getCreatureStats(target).isDead();
                        effectTick(target.getClass().getCreatureStats(target), target, EffectKey(*effectIt), magnitude);
                        bool isDead = target.getClass().getCreatureStats(target).isDead();

                        if (!wasDead && isDead)
                            MWBase::Environment::get().getMechanicsManager()->actorKilled(target, caster);
                    }
                    else
                    {
                        // add to list of active effects, to apply in next frame
                        ActiveSpells::ActiveEffect effect;
                        effect.mEffectId = effectIt->mEffectID;
                        effect.mArg = MWMechanics::EffectKey(*effectIt).mArg;
                        if (!hasDuration)
                            effect.mDuration = 1.0f;
                        else
                            effect.mDuration = static_cast<float>(effectIt->mDuration);
                        effect.mMagnitude = magnitude;

                        targetEffects.add(MWMechanics::EffectKey(*effectIt), MWMechanics::EffectParam(effect.mMagnitude));

                        appliedLastingEffects.push_back(effect);

                        // Command spells should have their effect, including taking the target out of combat, each time the spell successfully affects the target
                        if (((effectIt->mEffectID == ESM::MagicEffect::CommandHumanoid && target.getClass().isNpc())
                        || (effectIt->mEffectID == ESM::MagicEffect::CommandCreature && target.getTypeName() == typeid(ESM::Creature).name()))
                        && !caster.isEmpty() && caster.getClass().isActor() && target != getPlayer() && magnitude >= target.getClass().getCreatureStats(target).getLevel())
                        {
                            MWMechanics::AiFollow package(caster.getCellRef().getRefId(), true);
                            target.getClass().getCreatureStats(target).getAiSequence().stack(package, target);
                        }

                        // For absorb effects, also apply the effect to the caster - but with a negative
                        // magnitude, since we're transferring stats from the target to the caster
                        if (!caster.isEmpty() && caster.getClass().isActor())
                        {
                            for (int i=0; i<5; ++i)
                            {
                                if (effectIt->mEffectID == ESM::MagicEffect::AbsorbAttribute+i)
                                {
                                    std::vector<ActiveSpells::ActiveEffect> absorbEffects;
                                    ActiveSpells::ActiveEffect effect_ = effect;
                                    effect_.mMagnitude *= -1;
                                    absorbEffects.push_back(effect_);
                                    // Also make sure to set casterActorId = target, so that the effect on the caster gets purged when the target dies
                                    caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell("", true,
                                                absorbEffects, mSourceName, target.getClass().getCreatureStats(target).getActorId());
                                }
                            }
                        }
                    }
                }

                // Re-casting a summon effect will remove the creature from previous castings of that effect.
                if (isSummoningEffect(effectIt->mEffectID) && !target.isEmpty() && target.getClass().isActor())
                {
                    CreatureStats& targetStats = target.getClass().getCreatureStats(target);
                    std::map<CreatureStats::SummonKey, int>::iterator findCreature = targetStats.getSummonedCreatureMap().find(std::make_pair(effectIt->mEffectID, mId));
                    if (findCreature != targetStats.getSummonedCreatureMap().end())
                    {
                        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(target, findCreature->second);
                        targetStats.getSummonedCreatureMap().erase(findCreature);
                    }
                }

                if (target.getClass().isActor() || magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
                {
                    static const std::string schools[] = {
                        "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
                    };

                    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                    if(!magicEffect->mHitSound.empty())
                        sndMgr->playSound3D(target, magicEffect->mHitSound, 1.0f, 1.0f);
                    else
                        sndMgr->playSound3D(target, schools[magicEffect->mData.mSchool]+" hit", 1.0f, 1.0f);

                    // Add VFX
                    const ESM::Static* castStatic;
                    if (!magicEffect->mHit.empty())
                        castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
                    else
                        castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_DefaultHit");

                    std::string texture = magicEffect->mParticle;

                    // TODO: VFX are no longer active after saving/reloading the game
                    bool loop = (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
                    // Note: in case of non actor, a free effect should be fine as well
                    MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
                    if (anim)
                        anim->addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "", texture);
                }
            }
        }

        if (!exploded)
            MWBase::Environment::get().getWorld()->explodeSpell(mHitPosition, effects, caster, target, range, mId, mSourceName, mFromProjectile);

        if (!target.isEmpty()) {
            if (!reflectedEffects.mList.empty())
                inflict(caster, target, reflectedEffects, range, true, exploded);

            if (!appliedLastingEffects.empty())
            {
                int casterActorId = -1;
                if (!caster.isEmpty() && caster.getClass().isActor())
                    casterActorId = caster.getClass().getCreatureStats(caster).getActorId();
                target.getClass().getCreatureStats(target).getActiveSpells().addSpell(mId, mStack, appliedLastingEffects,
                                                                                      mSourceName, casterActorId);
            }
        }
    }

    bool CastSpell::applyInstantEffect(const MWWorld::Ptr &target, const MWWorld::Ptr &caster, const MWMechanics::EffectKey& effect, float magnitude)
    {
        short effectId = effect.mId;
        if (target.getClass().canLock(target))
        {
            if (effectId == ESM::MagicEffect::Lock)
            {
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                const ESM::MagicEffect *magiceffect = store.get<ESM::MagicEffect>().find(effectId);
                MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);     
                animation->addSpellCastGlow(magiceffect);
                if (target.getCellRef().getLockLevel() < magnitude) //If the door is not already locked to a higher value, lock it to spell magnitude
                {
                    if (caster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicLockSuccess}");
                    target.getClass().lock(target, static_cast<int>(magnitude));
                }
                return true;
            }
            else if (effectId == ESM::MagicEffect::Open)
            {
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                const ESM::MagicEffect *magiceffect = store.get<ESM::MagicEffect>().find(effectId);
                MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);     
                animation->addSpellCastGlow(magiceffect);
                if (target.getCellRef().getLockLevel() <= magnitude)
                {
                    if (target.getCellRef().getLockLevel() > 0)
                    {
                        MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock", 1.f, 1.f);
                        if (!caster.isEmpty() && caster.getClass().isActor())
                            MWBase::Environment::get().getMechanicsManager()->objectOpened(caster, target);

                        if (caster == getPlayer())
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicOpenSuccess}");
                    }
                    target.getClass().unlock(target);
                }
                else
                    MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock Fail", 1.f, 1.f);
                return true;
            }
        }
        else if (target.getClass().isActor() && effectId == ESM::MagicEffect::Dispel)
        {
            target.getClass().getCreatureStats(target).getActiveSpells().purgeAll(magnitude);
            return true;
        }
        else if (target.getClass().isActor() && target == getPlayer())
        {
            MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mCaster);

            if (effectId == ESM::MagicEffect::DivineIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "divinemarker");
                anim->removeEffect(ESM::MagicEffect::DivineIntervention);
                const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Summon_end");
                if (fx)
                    anim->addEffect("meshes\\" + fx->mModel, -1);
                return true;
            }
            else if (effectId == ESM::MagicEffect::AlmsiviIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "templemarker");
                anim->removeEffect(ESM::MagicEffect::AlmsiviIntervention);
                const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Summon_end");
                if (fx)
                    anim->addEffect("meshes\\" + fx->mModel, -1);
                return true;
            }
            else if (effectId == ESM::MagicEffect::Mark)
            {
                MWBase::Environment::get().getWorld()->getPlayer().markPosition(
                            target.getCell(), target.getRefData().getPosition());
                return true;
            }
            else if (effectId == ESM::MagicEffect::Recall)
            {
                MWWorld::CellStore* markedCell = NULL;
                ESM::Position markedPosition;

                MWBase::Environment::get().getWorld()->getPlayer().getMarkedPosition(markedCell, markedPosition);
                if (markedCell)
                {
                    MWWorld::ActionTeleport action(markedCell->isExterior() ? "" : markedCell->getCell()->mName,
                                            markedPosition, false);
                    action.execute(target);
                    anim->removeEffect(ESM::MagicEffect::Recall);
                }
                return true;
            }
        }
        return false;
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

    bool CastSpell::cast(const MWWorld::Ptr &item, bool launchProjectile)
    {
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            throw std::runtime_error("can't cast an item without an enchantment");

        mSourceName = item.getClass().getName(item);
        mId = item.getCellRef().getRefId();

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);

        mStack = false;

        bool godmode = mCaster == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        // Check if there's enough charge left
        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed || enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
        {
            int castCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), mCaster);

            if (item.getCellRef().getEnchantmentCharge() == -1)
                item.getCellRef().setEnchantmentCharge(static_cast<float>(enchantment->mData.mCharge));

            if (godmode)
                castCost = 0;

            if (item.getCellRef().getEnchantmentCharge() < castCost)
            {
                if (mCaster == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInsufficientCharge}");

                // Failure sound
                int school = 0;
                if (!enchantment->mEffects.mList.empty())
                {
                    short effectId = enchantment->mEffects.mList.front().mEffectID;
                    const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectId);
                    school = magicEffect->mData.mSchool;
                }
                static const std::string schools[] = {
                    "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
                };
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                sndMgr->playSound3D(mCaster, "Spell Failure " + schools[school], 1.0f, 1.0f);
                return false;
            }
            // Reduce charge
            item.getCellRef().setEnchantmentCharge(item.getCellRef().getEnchantmentCharge() - castCost);
        }

        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 1);
        }
        if (enchantment->mData.mType == ESM::Enchantment::CastOnce && !godmode)
        {
            item.getContainerStore()->remove(item, 1, mCaster);
        }
        else if (enchantment->mData.mType != ESM::Enchantment::WhenStrikes)
        {
            if (mCaster == getPlayer())
            {
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 3);
            }
        }

        inflict(mCaster, mCaster, enchantment->mEffects, ESM::RT_Self);

        bool isProjectile = false;
        if (item.getTypeName() == typeid(ESM::Weapon).name())
        {
            const MWWorld::LiveCellRef<ESM::Weapon> *ref = item.get<ESM::Weapon>();
            isProjectile = ref->mBase->mData.mType == ESM::Weapon::Arrow || ref->mBase->mData.mType == ESM::Weapon::Bolt || ref->mBase->mData.mType == ESM::Weapon::MarksmanThrown;
        }

        if (isProjectile || !mTarget.isEmpty())
            inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Touch);

        if (launchProjectile)
            launchMagicBolt(enchantment->mEffects);
        else if (isProjectile || !mTarget.isEmpty())
            inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Target);

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

        bool godmode = mCaster == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        if (mCaster.getClass().isActor() && !mAlwaysSucceed)
        {
            school = getSpellSchool(spell, mCaster);

            CreatureStats& stats = mCaster.getClass().getCreatureStats(mCaster);

            if (!godmode)
            {
                // Reduce fatigue (note that in the vanilla game, both GMSTs are 0, and there's no fatigue loss)
                static const float fFatigueSpellBase = store.get<ESM::GameSetting>().find("fFatigueSpellBase")->getFloat();
                static const float fFatigueSpellMult = store.get<ESM::GameSetting>().find("fFatigueSpellMult")->getFloat();
                DynamicStat<float> fatigue = stats.getFatigue();
                const float normalizedEncumbrance = mCaster.getClass().getNormalizedEncumbrance(mCaster);

                float fatigueLoss = spell->mData.mCost * (fFatigueSpellBase + normalizedEncumbrance * fFatigueSpellMult);
                fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss); stats.setFatigue(fatigue);

                bool fail = false;

                // Check success
                if (!(mCaster == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState()))
                {
                    float successChance = getSpellSuccessChance(spell, mCaster);
                    if (Misc::Rng::roll0to99() >= successChance)
                    {
                        if (mCaster == getPlayer())
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicSkillFail}");
                        fail = true;
                    }
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

            // A power can be used once per 24h
            if (spell->mData.mType == ESM::Spell::ST_Power)
                stats.getSpells().usePower(spell);
        }

        if (mCaster == getPlayer() && spellIncreasesSkill(spell))
            mCaster.getClass().skillUsageSucceeded(mCaster,
                spellSchoolToSkill(school), 0);
    
        // A non-actor doesn't play its spell cast effects from a character controller, so play them here
        if (!mCaster.getClass().isActor())
            playSpellCastingEffects(mId);

        inflict(mCaster, mCaster, spell->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
            inflict(mTarget, mCaster, spell->mEffects, ESM::RT_Touch);

        launchMagicBolt(spell->mEffects);

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
                    0.2f * creatureStats.getAttribute (ESM::Attribute::Intelligence).getModified()
                    + 0.1f * creatureStats.getAttribute (ESM::Attribute::Luck).getModified())
                    * creatureStats.getFatigueTerm();

        int roll = Misc::Rng::roll0to99();
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
        y *= 0.25f * x;
        if (magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
            effect.mDuration = static_cast<int>(y);
        else
            effect.mDuration = 1;
        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
        {
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
                magnitude = floor((0.05f * y) / (0.1f * magicEffect->mData.mBaseCost));
            else
                magnitude = floor(y / (0.1f * magicEffect->mData.mBaseCost));
            magnitude = std::max(1.f, magnitude);
        }
        else
            magnitude = 1;

        effect.mMagnMax = static_cast<int>(magnitude);
        effect.mMagnMin = static_cast<int>(magnitude);

        ESM::EffectList effects;
        effects.mList.push_back(effect);

        inflict(mCaster, mCaster, effects, ESM::RT_Self);

        return true;
    }

    void CastSpell::playSpellCastingEffects(const std::string &spellid){
       
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Spell *spell = store.get<ESM::Spell>().find(spellid);

        for (std::vector<ESM::ENAMstruct>::const_iterator iter = spell->mEffects.mList.begin();
            iter != spell->mEffects.mList.end(); ++iter)
        {
            const ESM::MagicEffect *effect;
            effect = store.get<ESM::MagicEffect>().find(iter->mEffectID);

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(mCaster);

            if (animation && mCaster.getClass().isActor()) // TODO: Non-actors should also create a spell cast vfx even if they are disabled (animation == NULL)
            {
                const ESM::Static* castStatic;

                if (!effect->mCasting.empty())
                    castStatic = store.get<ESM::Static>().find (effect->mCasting);
                else
                    castStatic = store.get<ESM::Static>().find ("VFX_DefaultCast");

                std::string texture = effect->mParticle;

                animation->addEffect("meshes\\" + castStatic->mModel, effect->mIndex, false, "", texture);
            }

            if (animation && !mCaster.getClass().isActor())
                animation->addSpellCastGlow(effect);

            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            if(!effect->mCastSound.empty())
                sndMgr->playSound3D(mCaster, effect->mCastSound, 1.0f, 1.0f);
            else
                sndMgr->playSound3D(mCaster, schools[effect->mData.mSchool]+" cast", 1.0f, 1.0f);
        }
    }

    int getEffectiveEnchantmentCastCost(float castCost, const MWWorld::Ptr &actor)
    {
        /*
         * Each point of enchant skill above/under 10 subtracts/adds
         * one percent of enchantment cost while minimum is 1.
         */
        int eSkill = actor.getClass().getSkill(actor, ESM::Skill::Enchant);
        const float result = castCost - (castCost / 100) * (eSkill - 10);

        return static_cast<int>((result < 1) ? 1 : result);
    }

    bool isSummoningEffect(int effectId)
    {
        return ((effectId >= ESM::MagicEffect::SummonScamp
                && effectId <= ESM::MagicEffect::SummonStormAtronach)
                || effectId == ESM::MagicEffect::SummonCenturionSphere
                || (effectId >= ESM::MagicEffect::SummonFabricant
                    && effectId <= ESM::MagicEffect::SummonCreature05));
    }

    bool disintegrateSlot (MWWorld::Ptr ptr, int slot, float disintegrate)
    {
        if (ptr.getClass().hasInventoryStore(ptr))
        {
            MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
            MWWorld::ContainerStoreIterator item =
                    inv.getSlot(slot);

            if (item != inv.end() && (item.getType() == MWWorld::ContainerStore::Type_Armor || item.getType() == MWWorld::ContainerStore::Type_Weapon))
            {
                if (!item->getClass().hasItemHealth(*item))
                    return false;
                int charge = item->getClass().getItemHealth(*item);

                if (charge == 0)
                    return false;

                // Store remainder of disintegrate amount (automatically subtracted if > 1)
                item->getCellRef().applyChargeRemainderToBeSubtracted(disintegrate - std::floor(disintegrate));

                charge = item->getClass().getItemHealth(*item);
                charge -=
                        std::min(static_cast<int>(disintegrate),
                                 charge);
                item->getCellRef().setCharge(charge);

                if (charge == 0)
                {
                    // Will unequip the broken item and try to find a replacement
                    if (ptr != getPlayer())
                        inv.autoEquip(ptr);
                    else
                        inv.unequipItem(*item, ptr);
                }

                return true;
            }
        }
        return false;
    }

    void adjustDynamicStat(CreatureStats& creatureStats, int index, float magnitude)
    {
        DynamicStat<float> stat = creatureStats.getDynamic(index);
        stat.setCurrent(stat.getCurrent() + magnitude, index == 2);
        creatureStats.setDynamic(index, stat);
    }

    bool effectTick(CreatureStats& creatureStats, const MWWorld::Ptr& actor, const EffectKey &effectKey, float magnitude)
    {
        if (magnitude == 0.f)
            return false;

        bool receivedMagicDamage = false;

        bool godmode = actor == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState(); 

        switch (effectKey.mId)
        {
        case ESM::MagicEffect::DamageAttribute:
        {
            AttributeValue attr = creatureStats.getAttribute(effectKey.mArg);
            attr.damage(magnitude);
            creatureStats.setAttribute(effectKey.mArg, attr);
            break;
        }
        case ESM::MagicEffect::RestoreAttribute:
        {
            AttributeValue attr = creatureStats.getAttribute(effectKey.mArg);
            attr.restore(magnitude);
            creatureStats.setAttribute(effectKey.mArg, attr);
            break;
        }
        case ESM::MagicEffect::RestoreHealth:
        case ESM::MagicEffect::RestoreMagicka:
        case ESM::MagicEffect::RestoreFatigue:
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::RestoreHealth, magnitude);
            break;
        case ESM::MagicEffect::DamageHealth:
            if (!godmode)
            {
                receivedMagicDamage = true;
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::DamageHealth, -magnitude);
            }
        case ESM::MagicEffect::DamageMagicka:
        case ESM::MagicEffect::DamageFatigue:
            if (!godmode)
            {
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::DamageHealth, -magnitude);
            }

            break;

        case ESM::MagicEffect::AbsorbHealth:
            if (!godmode)
            {
                if (magnitude > 0.f)
                    receivedMagicDamage = true;
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);
            }
        case ESM::MagicEffect::AbsorbMagicka:
        case ESM::MagicEffect::AbsorbFatigue:
            if (!godmode)
            {
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);
            }

            break;

        case ESM::MagicEffect::DisintegrateArmor:
        {
            // According to UESP
            int priorities[] = {
                MWWorld::InventoryStore::Slot_CarriedLeft,
                MWWorld::InventoryStore::Slot_Cuirass,
                MWWorld::InventoryStore::Slot_LeftPauldron,
                MWWorld::InventoryStore::Slot_RightPauldron,
                MWWorld::InventoryStore::Slot_LeftGauntlet,
                MWWorld::InventoryStore::Slot_RightGauntlet,
                MWWorld::InventoryStore::Slot_Helmet,
                MWWorld::InventoryStore::Slot_Greaves,
                MWWorld::InventoryStore::Slot_Boots
            };

            for (unsigned int i=0; i<sizeof(priorities)/sizeof(int); ++i)
            {
                if (disintegrateSlot(actor, priorities[i], magnitude))
                    break;
            }
            break;
        }
        case ESM::MagicEffect::DisintegrateWeapon:
            disintegrateSlot(actor, MWWorld::InventoryStore::Slot_CarriedRight, magnitude);
            break;

        case ESM::MagicEffect::SunDamage:
        {
            // isInCell shouldn't be needed, but updateActor called during game start
            if (!actor.isInCell() || !actor.getCell()->isExterior())
                break;
            float time = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
            float timeDiff = std::min(7.f, std::max(0.f, std::abs(time - 13)));
            float damageScale = 1.f - timeDiff / 7.f;
            // When cloudy, the sun damage effect is halved
            static float fMagicSunBlockedMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fMagicSunBlockedMult")->getFloat();

            int weather = MWBase::Environment::get().getWorld()->getCurrentWeather();
            if (weather > 1)
                damageScale *= fMagicSunBlockedMult;

            if (!godmode)
            {
                adjustDynamicStat(creatureStats, 0, -magnitude * damageScale);
                if (magnitude * damageScale > 0.f)
                    receivedMagicDamage = true;
            }

            break;
        }

        case ESM::MagicEffect::FireDamage:
        case ESM::MagicEffect::ShockDamage:
        case ESM::MagicEffect::FrostDamage:
        case ESM::MagicEffect::Poison:
        {
            if (!godmode)
            {
                adjustDynamicStat(creatureStats, 0, -magnitude);
                receivedMagicDamage = true;
            }

            break;
        }

        case ESM::MagicEffect::DamageSkill:
        case ESM::MagicEffect::RestoreSkill:
        {
            if (!actor.getClass().isNpc())
                break;
            NpcStats &npcStats = actor.getClass().getNpcStats(actor);
            SkillValue& skill = npcStats.getSkill(effectKey.mArg);
            if (effectKey.mId == ESM::MagicEffect::RestoreSkill)
                skill.restore(magnitude);
            else
                skill.damage(magnitude);
            break;
        }

        case ESM::MagicEffect::CurePoison:
            actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(ESM::MagicEffect::Poison);
            break;
        case ESM::MagicEffect::CureParalyzation:
            actor.getClass().getCreatureStats(actor).getActiveSpells().purgeEffect(ESM::MagicEffect::Paralyze);
            break;
        case ESM::MagicEffect::CureCommonDisease:
            actor.getClass().getCreatureStats(actor).getSpells().purgeCommonDisease();
            break;
        case ESM::MagicEffect::CureBlightDisease:
            actor.getClass().getCreatureStats(actor).getSpells().purgeBlightDisease();
            break;
        case ESM::MagicEffect::CureCorprusDisease:
            actor.getClass().getCreatureStats(actor).getSpells().purgeCorprusDisease();
            break;
        case ESM::MagicEffect::RemoveCurse:
            actor.getClass().getCreatureStats(actor).getSpells().purgeCurses();
            break;
        default:
            return false;
        }

        if (receivedMagicDamage && actor == getPlayer())
            MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
        return true;
    }

    std::string getSummonedCreature(int effectId)
    {
        static std::map<int, std::string> summonMap;
        if (summonMap.empty())
        {
            summonMap[ESM::MagicEffect::SummonAncestralGhost] = "sMagicAncestralGhostID";
            summonMap[ESM::MagicEffect::SummonBonelord] = "sMagicBonelordID";
            summonMap[ESM::MagicEffect::SummonBonewalker] = "sMagicLeastBonewalkerID";
            summonMap[ESM::MagicEffect::SummonCenturionSphere] = "sMagicCenturionSphereID";
            summonMap[ESM::MagicEffect::SummonClannfear] = "sMagicClannfearID";
            summonMap[ESM::MagicEffect::SummonDaedroth] = "sMagicDaedrothID";
            summonMap[ESM::MagicEffect::SummonDremora] = "sMagicDremoraID";
            summonMap[ESM::MagicEffect::SummonFabricant] = "sMagicFabricantID";
            summonMap[ESM::MagicEffect::SummonFlameAtronach] = "sMagicFlameAtronachID";
            summonMap[ESM::MagicEffect::SummonFrostAtronach] = "sMagicFrostAtronachID";
            summonMap[ESM::MagicEffect::SummonGoldenSaint] = "sMagicGoldenSaintID";
            summonMap[ESM::MagicEffect::SummonGreaterBonewalker] = "sMagicGreaterBonewalkerID";
            summonMap[ESM::MagicEffect::SummonHunger] = "sMagicHungerID";
            summonMap[ESM::MagicEffect::SummonScamp] = "sMagicScampID";
            summonMap[ESM::MagicEffect::SummonSkeletalMinion] = "sMagicSkeletalMinionID";
            summonMap[ESM::MagicEffect::SummonStormAtronach] = "sMagicStormAtronachID";
            summonMap[ESM::MagicEffect::SummonWingedTwilight] = "sMagicWingedTwilightID";
            summonMap[ESM::MagicEffect::SummonWolf] = "sMagicCreature01ID";
            summonMap[ESM::MagicEffect::SummonBear] = "sMagicCreature02ID";
            summonMap[ESM::MagicEffect::SummonBonewolf] = "sMagicCreature03ID";
            summonMap[ESM::MagicEffect::SummonCreature04] = "sMagicCreature04ID";
            summonMap[ESM::MagicEffect::SummonCreature05] = "sMagicCreature05ID";
        }

        std::map<int, std::string>::const_iterator it = summonMap.find(effectId);
        if (it == summonMap.end())
            return std::string();
        else
            return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(it->second)->getString();
    }

}
