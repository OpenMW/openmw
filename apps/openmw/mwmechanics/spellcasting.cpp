#include "spellcasting.hpp"

#include <limits>
#include <iomanip>

#include <components/misc/constants.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/settings.hpp>

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

#include "npcstats.hpp"
#include "actorutil.hpp"
#include "aifollow.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
    ESM::Skill::SkillEnum spellSchoolToSkill(int school)
    {
        static const std::array<ESM::Skill::SkillEnum, 6> schoolSkillArray
        {
            ESM::Skill::Alteration, ESM::Skill::Conjuration, ESM::Skill::Destruction,
            ESM::Skill::Illusion, ESM::Skill::Mysticism, ESM::Skill::Restoration
        };
        return schoolSkillArray.at(school);
    }

    float calcEffectCost(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect)
    {
        if (!magicEffect)
            magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);
        bool hasMagnitude = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude);
        bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
        int minMagn = hasMagnitude ? effect.mMagnMin : 1;
        int maxMagn = hasMagnitude ? effect.mMagnMax : 1;
        int duration = hasDuration ? effect.mDuration : 1;
        static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore()
            .get<ESM::GameSetting>().find("fEffectCostMult")->mValue.getFloat();

        float x = 0.5 * (std::max(1, minMagn) + std::max(1, maxMagn));
        x *= 0.1 * magicEffect->mData.mBaseCost;
        x *= 1 + duration;
        x += 0.05 * std::max(1, effect.mArea) * magicEffect->mData.mBaseCost;

        return x * fEffectCostMult;
    }

    float calcSpellBaseSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool)
    {
        // Morrowind for some reason uses a formula slightly different from magicka cost calculation
        float y = std::numeric_limits<float>::max();
        float lowestSkill = 0;

        for (const ESM::ENAMstruct& effect : spell->mEffects.mList)
        {
            float x = static_cast<float>(effect.mDuration);
            const auto magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce))
                x = std::max(1.f, x);

            x *= 0.1f * magicEffect->mData.mBaseCost;
            x *= 0.5f * (effect.mMagnMin + effect.mMagnMax);
            x += effect.mArea * 0.05f * magicEffect->mData.mBaseCost;
            if (effect.mRange == ESM::RT_Target)
                x *= 1.5f;
            static const float fEffectCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fEffectCostMult")->mValue.getFloat();
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

        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        int actorWillpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        int actorLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();

        float castChance = (lowestSkill - spell->mData.mCost + 0.2f * actorWillpower + 0.1f * actorLuck);

        return castChance;
    }

    float getSpellSuccessChance (const ESM::Spell* spell, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap, bool checkMagicka)
    {
        bool godmode = actor == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        float castBonus = -stats.getMagicEffects().get(ESM::MagicEffect::Sound).getMagnitude();

        float castChance = calcSpellBaseSuccessChance(spell, actor, effectiveSchool) + castBonus;
        castChance *= stats.getFatigueTerm();

        if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).getMagnitude()&& !godmode)
            return 0;

        if (spell->mData.mType == ESM::Spell::ST_Power)
            return stats.getSpells().canUsePower(spell) ? 100 : 0;

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 100;

        if (checkMagicka && stats.getMagicka().getCurrent() < spell->mData.mCost && !godmode)
            return 0;

        if (spell->mData.mFlags & ESM::Spell::F_Always)
            return 100;

        if (godmode)
            return 100;

        return std::max(0.f, cap ? std::min(100.f, castChance) : castChance);
    }

    float getSpellSuccessChance (const std::string& spellId, const MWWorld::Ptr& actor, int* effectiveSchool, bool cap, bool checkMagicka)
    {
        if (const auto spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId))
            return getSpellSuccessChance(spell, actor, effectiveSchool, cap, checkMagicka);
        return 0.f;
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
        return spell->mData.mType == ESM::Spell::ST_Spell && !(spell->mData.mFlags & ESM::Spell::F_Always);
    }

    bool spellIncreasesSkill(const std::string &spellId)
    {
        const auto spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(spellId);
        return spell && spellIncreasesSkill(spell);
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

        // Effects with no resistance attribute belonging to them can not be resisted
        if (ESM::MagicEffect::getResistanceEffect(effectId) == -1)
            return 0.f;

        float resistance = getEffectResistanceAttribute(effectId, magicEffects);

        int willpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        float luck = static_cast<float>(stats.getAttribute(ESM::Attribute::Luck).getModified());
        float x = (willpower + 0.1f * luck) * stats.getFatigueTerm();

        // This makes spells that are easy to cast harder to resist and vice versa
        float castChance = 100.f;
        if (spell != nullptr && !caster.isEmpty() && caster.getClass().isActor())
        {
            castChance = getSpellSuccessChance(spell, caster, nullptr, false, false); // Uncapped casting chance
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
        return x;
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

    class GetAbsorptionProbability : public MWMechanics::EffectSourceVisitor
    {
    public:
        float mProbability{0.f};

        GetAbsorptionProbability() = default;

        virtual void visit (MWMechanics::EffectKey key,
                                const std::string& sourceName, const std::string& sourceId, int casterActorId,
                            float magnitude, float remainingTime = -1, float totalTime = -1)
        {
            if (key.mId == ESM::MagicEffect::SpellAbsorption)
            {
                if (mProbability == 0.f)
                    mProbability = magnitude / 100;
                else
                {
                    // If there are different sources of SpellAbsorption effect, multiply failing probability for all effects.
                    // Real absorption probability will be the (1 - total fail chance) in this case.
                    float failProbability = 1.f - mProbability;
                    failProbability *= 1.f - magnitude / 100;
                    mProbability = 1.f - failProbability;
                }
            }
        }
    };

    CastSpell::CastSpell(const MWWorld::Ptr &caster, const MWWorld::Ptr &target, const bool fromProjectile, const bool manualSpell)
        : mCaster(caster)
        , mTarget(target)
        , mFromProjectile(fromProjectile)
        , mManualSpell(manualSpell)
    {
    }

    void CastSpell::launchMagicBolt ()
    {
        osg::Vec3f fallbackDirection(0, 1, 0);
        osg::Vec3f offset(0, 0, 0);
        if (!mTarget.isEmpty() && mTarget.getClass().isActor())
            offset.z() = MWBase::Environment::get().getWorld()->getHalfExtents(mTarget).z();

        // Fall back to a "caster to target" direction if we have no other means of determining it
        // (e.g. when cast by a non-actor)
        if (!mTarget.isEmpty())
            fallbackDirection =
                (mTarget.getRefData().getPosition().asVec3() + offset) -
                (mCaster.getRefData().getPosition().asVec3());

        MWBase::Environment::get().getWorld()->launchMagicBolt(mId, mCaster, fallbackDirection);
    }

    void CastSpell::inflict(const MWWorld::Ptr &target, const MWWorld::Ptr &caster,
                            const ESM::EffectList &effects, ESM::RangeType range, bool reflected, bool exploded)
    {
        if (!target.isEmpty() && target.getClass().isActor() && target.getClass().getCreatureStats(target).isDead())
            return;

        // If none of the effects need to apply, we can early-out
        bool found = false;
        for (const ESM::ENAMstruct& effect : effects.mList)
        {
            if (effect.mRange == range)
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
            // Unlike Reflect, this is done once per spell absorption effect source
            bool absorbed = false;
            if (spell && caster != target && target.getClass().isActor())
            {
                CreatureStats& stats = target.getClass().getCreatureStats(target);
                if (stats.getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).getMagnitude() > 0.f)
                {
                    GetAbsorptionProbability check;
                    stats.getActiveSpells().visitEffectSources(check);
                    stats.getSpells().visitEffectSources(check);
                    if (target.getClass().hasInventoryStore(target))
                        target.getClass().getInventoryStore(target).visitEffectSources(check);

                    int absorb = check.mProbability * 100;
                    absorbed = (Misc::Rng::roll0to99() < absorb);
                    if (absorbed)
                    {
                        const ESM::Static* absorbStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Absorb");
                        MWBase::Environment::get().getWorld()->getAnimation(target)->addEffect(
                                    "meshes\\" + absorbStatic->mModel, ESM::MagicEffect::SpellAbsorption, false, "");
                        // Magicka is increased by cost of spell
                        DynamicStat<float> magicka = stats.getMagicka();
                        magicka.setCurrent(magicka.getCurrent() + spell->mData.mCost);
                        stats.setMagicka(magicka);
                    }
                }
            }

            float magnitudeMult = 1;

            if (target.getClass().isActor())
            {
                if (absorbed)
                    continue;

                bool isHarmful = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful;
                // Reflect harmful effects
                if (isHarmful && !reflected && !caster.isEmpty() && caster != target && !(magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable))
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
                else if (isHarmful && castByPlayer && target != caster)
                {
                    // If player is attempting to cast a harmful spell and it wasn't fully resisted, show the target's HP bar
                    MWBase::Environment::get().getWindowManager()->setEnemy(target);
                }

                if (target == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState() && isHarmful)
                    magnitudeMult = 0;

                // Notify the target actor they've been hit
                if (target != caster && !caster.isEmpty() && isHarmful)
                    target.getClass().onHit(target, 0.0f, true, MWWorld::Ptr(), caster, osg::Vec3f(), true);
            }

            if (magnitudeMult > 0 && !absorbed)
            {
                float magnitude = effectIt->mMagnMin + Misc::Rng::rollDice(effectIt->mMagnMax - effectIt->mMagnMin + 1);
                magnitude *= magnitudeMult;

                if (!target.getClass().isActor())
                {
                    // non-actor objects have no list of active magic effects, so have to apply instantly
                    if (!applyInstantEffect(target, caster, EffectKey(*effectIt), magnitude))
                        continue;
                }
                else // target.getClass().isActor() == true
                {
                    ActiveSpells::ActiveEffect effect;
                    effect.mEffectId = effectIt->mEffectID;
                    effect.mArg = MWMechanics::EffectKey(*effectIt).mArg;
                    effect.mMagnitude = magnitude;

                    // Avoid applying absorb effects if the caster is the target
                    // We still need the spell to be added
                    if (caster == target
                        && effectIt->mEffectID >= ESM::MagicEffect::AbsorbAttribute
                        && effectIt->mEffectID <= ESM::MagicEffect::AbsorbSkill)
                    {
                        effect.mMagnitude = 0;
                    }

                    bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
                    if (hasDuration && effectIt->mDuration == 0)
                    {
                        // We still should add effect to list to allow GetSpellEffects to detect this spell
                        effect.mDuration = 0.f;
                        appliedLastingEffects.push_back(effect);

                        // duration 0 means apply full magnitude instantly
                        bool wasDead = target.getClass().getCreatureStats(target).isDead();
                        effectTick(target.getClass().getCreatureStats(target), target, EffectKey(*effectIt), magnitude);
                        bool isDead = target.getClass().getCreatureStats(target).isDead();

                        if (!wasDead && isDead)
                            MWBase::Environment::get().getMechanicsManager()->actorKilled(target, caster);
                    }
                    else
                    {
                        effect.mDuration = hasDuration ? static_cast<float>(effectIt->mDuration) : 1.f;

                        targetEffects.add(MWMechanics::EffectKey(*effectIt), MWMechanics::EffectParam(effect.mMagnitude));

                        // add to list of active effects, to apply in next frame
                        appliedLastingEffects.push_back(effect);

                        // Unequip all items, if a spell with the ExtraSpell effect was casted
                        if (effectIt->mEffectID == ESM::MagicEffect::ExtraSpell && target.getClass().hasInventoryStore(target))
                        {
                            MWWorld::InventoryStore& store = target.getClass().getInventoryStore(target);
                            store.unequipAll(target);
                        }

                        // Command spells should have their effect, including taking the target out of combat, each time the spell successfully affects the target
                        if (((effectIt->mEffectID == ESM::MagicEffect::CommandHumanoid && target.getClass().isNpc())
                        || (effectIt->mEffectID == ESM::MagicEffect::CommandCreature && target.getTypeName() == typeid(ESM::Creature).name()))
                        && !caster.isEmpty() && caster.getClass().isActor() && target != getPlayer() && magnitude >= target.getClass().getCreatureStats(target).getLevel())
                        {
                            MWMechanics::AiFollow package(caster, true);
                            target.getClass().getCreatureStats(target).getAiSequence().stack(package, target);
                        }

                        // For absorb effects, also apply the effect to the caster - but with a negative
                        // magnitude, since we're transferring stats from the target to the caster
                        if (!caster.isEmpty() && caster != target && caster.getClass().isActor())
                        {
                            if (effectIt->mEffectID >= ESM::MagicEffect::AbsorbAttribute &&
                                effectIt->mEffectID <= ESM::MagicEffect::AbsorbSkill)
                            {
                                std::vector<ActiveSpells::ActiveEffect> absorbEffects;
                                ActiveSpells::ActiveEffect effect_ = effect;
                                effect_.mMagnitude *= -1;
                                absorbEffects.push_back(effect_);
                                if (reflected && Settings::Manager::getBool("classic reflected absorb spells behavior", "Game"))
                                    target.getClass().getCreatureStats(target).getActiveSpells().addSpell("", true,
                                        absorbEffects, mSourceName, caster.getClass().getCreatureStats(caster).getActorId());
                                else
                                    caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell("", true,
                                        absorbEffects, mSourceName, target.getClass().getCreatureStats(target).getActorId());
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

                    bool loop = (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
                    // Note: in case of non actor, a free effect should be fine as well
                    MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
                    if (anim && !castStatic->mModel.empty())
                        anim->addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "", magicEffect->mParticle);
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
                if (animation)
                    animation->addSpellCastGlow(magiceffect);
                if (target.getCellRef().getLockLevel() < magnitude) //If the door is not already locked to a higher value, lock it to spell magnitude
                {
                    if (caster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicLockSuccess}");
                    target.getCellRef().lock(static_cast<int>(magnitude));
                }
                return true;
            }
            else if (effectId == ESM::MagicEffect::Open)
            {
                if (!caster.isEmpty())
                {
                    MWBase::Environment::get().getMechanicsManager()->unlockAttempted(getPlayer(), target);
                    // Use the player instead of the caster for vanilla crime compatibility
                }
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                const ESM::MagicEffect *magiceffect = store.get<ESM::MagicEffect>().find(effectId);
                MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);
                if (animation)
                    animation->addSpellCastGlow(magiceffect);
                if (target.getCellRef().getLockLevel() <= magnitude)
                {
                    if (target.getCellRef().getLockLevel() > 0)
                    {
                        MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock", 1.f, 1.f);

                        if (caster == getPlayer())
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicOpenSuccess}");
                    }
                    target.getCellRef().unlock();
                }
                else
                {
                    MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock Fail", 1.f, 1.f);
                }

                return true;
            }
        }
        else if (target.getClass().isActor() && effectId == ESM::MagicEffect::Dispel)
        {
            target.getClass().getCreatureStats(target).getActiveSpells().purgeAll(magnitude, true);
            return true;
        }
        else if (target.getClass().isActor() && target == getPlayer())
        {
            MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mCaster);
            bool teleportingEnabled = MWBase::Environment::get().getWorld()->isTeleportingEnabled();

            if (effectId == ESM::MagicEffect::DivineIntervention || effectId == ESM::MagicEffect::AlmsiviIntervention)
            {
                if (!teleportingEnabled)
                {
                    if (caster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                    return true;
                }
                std::string marker = (effectId == ESM::MagicEffect::DivineIntervention) ? "divinemarker" : "templemarker";
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, marker);
                anim->removeEffect(effectId);
                const ESM::Static* fx = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>()
                    .search("VFX_Summon_end");
                if (fx)
                    anim->addEffect("meshes\\" + fx->mModel, -1);
                return true;
            }
            else if (effectId == ESM::MagicEffect::Mark)
            {
                if (teleportingEnabled)
                {
                    MWBase::Environment::get().getWorld()->getPlayer().markPosition(
                                target.getCell(), target.getRefData().getPosition());
                }
                else if (caster == getPlayer())
                {
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                }
                return true;
            }
            else if (effectId == ESM::MagicEffect::Recall)
            {
                if (!teleportingEnabled)
                {
                    if (caster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                    return true;
                }

                MWWorld::CellStore* markedCell = nullptr;
                ESM::Position markedPosition;

                MWBase::Environment::get().getWorld()->getPlayer().getMarkedPosition(markedCell, markedPosition);
                if (markedCell)
                {
                    MWWorld::ActionTeleport action(markedCell->isExterior() ? "" : markedCell->getCell()->mName,
                                            markedPosition, false);
                    action.execute(target);
                    anim->removeEffect(effectId);
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
        bool isProjectile = false;
        if (item.getTypeName() == typeid(ESM::Weapon).name())
        {
            int type = item.get<ESM::Weapon>()->mBase->mData.mType;
            ESM::WeaponType::Class weapclass = MWMechanics::getWeaponType(type)->mWeaponClass;
            isProjectile = (weapclass == ESM::WeaponType::Thrown || weapclass == ESM::WeaponType::Ammo);
        }
        int type = enchantment->mData.mType;

        // Check if there's enough charge left
        if (!godmode && (type == ESM::Enchantment::WhenUsed || (!isProjectile && type == ESM::Enchantment::WhenStrikes)))
        {
            int castCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), mCaster);

            if (item.getCellRef().getEnchantmentCharge() == -1)
                item.getCellRef().setEnchantmentCharge(static_cast<float>(enchantment->mData.mCharge));

            if (item.getCellRef().getEnchantmentCharge() < castCost)
            {
                if (mCaster == getPlayer())
                {
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
                }
                return false;
            }
            // Reduce charge
            item.getCellRef().setEnchantmentCharge(item.getCellRef().getEnchantmentCharge() - castCost);
        }

        if (type == ESM::Enchantment::WhenUsed)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 1);
        }
        else if (type == ESM::Enchantment::CastOnce)
        {
            if (!godmode)
                item.getContainerStore()->remove(item, 1, mCaster);
        }
        else if (type == ESM::Enchantment::WhenStrikes)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 3);
        }

        inflict(mCaster, mCaster, enchantment->mEffects, ESM::RT_Self);

        if (isProjectile || !mTarget.isEmpty())
            inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Touch);

        if (launchProjectile)
            launchMagicBolt();
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

        if (mCaster.getClass().isActor() && !mAlwaysSucceed && !mManualSpell)
        {
            school = getSpellSchool(spell, mCaster);

            CreatureStats& stats = mCaster.getClass().getCreatureStats(mCaster);

            if (!godmode)
            {
                // Reduce fatigue (note that in the vanilla game, both GMSTs are 0, and there's no fatigue loss)
                static const float fFatigueSpellBase = store.get<ESM::GameSetting>().find("fFatigueSpellBase")->mValue.getFloat();
                static const float fFatigueSpellMult = store.get<ESM::GameSetting>().find("fFatigueSpellMult")->mValue.getFloat();
                DynamicStat<float> fatigue = stats.getFatigue();
                const float normalizedEncumbrance = mCaster.getClass().getNormalizedEncumbrance(mCaster);

                float fatigueLoss = spell->mData.mCost * (fFatigueSpellBase + normalizedEncumbrance * fFatigueSpellMult);
                fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss); 
                stats.setFatigue(fatigue);

                bool fail = false;

                // Check success
                float successChance = getSpellSuccessChance(spell, mCaster, nullptr, true, false);
                if (Misc::Rng::roll0to99() >= successChance)
                {
                    if (mCaster == getPlayer())
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

            // A power can be used once per 24h
            if (spell->mData.mType == ESM::Spell::ST_Power)
                stats.getSpells().usePower(spell);
        }

        if (mCaster == getPlayer() && spellIncreasesSkill())
            mCaster.getClass().skillUsageSucceeded(mCaster,
                spellSchoolToSkill(school), 0);
    
        // A non-actor doesn't play its spell cast effects from a character controller, so play them here
        if (!mCaster.getClass().isActor())
            playSpellCastingEffects(spell->mEffects.mList);

        inflict(mCaster, mCaster, spell->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
            inflict(mTarget, mCaster, spell->mEffects, ESM::RT_Touch);

        launchMagicBolt();

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

        const MWMechanics::CreatureStats& creatureStats = mCaster.getClass().getCreatureStats(mCaster);

        float x = (mCaster.getClass().getSkill(mCaster, ESM::Skill::Alchemy) +
                    0.2f * creatureStats.getAttribute (ESM::Attribute::Intelligence).getModified()
                    + 0.1f * creatureStats.getAttribute (ESM::Attribute::Luck).getModified())
                    * creatureStats.getFatigueTerm();

        int roll = Misc::Rng::roll0to99();
        if (roll > x)
        {
            // "X has no effect on you"
            std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage50")->mValue.getString();
            message = Misc::StringUtils::format(message, ingredient->mName);
            MWBase::Environment::get().getWindowManager()->messageBox(message);
            return false;
        }

        float magnitude = 0;
        float y = roll / std::min(x, 100.f);
        y *= 0.25f * x;
        if (magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
            effect.mDuration = 1;
        else
            effect.mDuration = static_cast<int>(y);
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

    void CastSpell::playSpellCastingEffects(const std::string &spellid, bool enchantment)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        if (enchantment)
        {
            if (const auto spell = store.get<ESM::Enchantment>().search(spellid))
                playSpellCastingEffects(spell->mEffects.mList);
        }
        else
        {
            if (const auto spell = store.get<ESM::Spell>().search(spellid))
                playSpellCastingEffects(spell->mEffects.mList);
        }
    }

    void CastSpell::playSpellCastingEffects(const std::vector<ESM::ENAMstruct>& effects)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        std::vector<std::string> addedEffects;
        for (const ESM::ENAMstruct& effectData : effects)
        {
            const auto effect = store.get<ESM::MagicEffect>().find(effectData.mEffectID);

            const ESM::Static* castStatic;

            if (!effect->mCasting.empty())
                castStatic = store.get<ESM::Static>().find (effect->mCasting);
            else
                castStatic = store.get<ESM::Static>().find ("VFX_DefaultCast");

            // check if the effect was already added
            if (std::find(addedEffects.begin(), addedEffects.end(), "meshes\\" + castStatic->mModel) != addedEffects.end())
                continue;

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(mCaster);
            if (animation)
            {
                animation->addEffect("meshes\\" + castStatic->mModel, effect->mIndex, false, "", effect->mParticle);
            }
            else
            {
                // If the caster has no animation, add the effect directly to the effectManager
                // We should scale it manually
                osg::Vec3f bounds (MWBase::Environment::get().getWorld()->getHalfExtents(mCaster) * 2.f / Constants::UnitsPerFoot);
                float scale = std::max({ bounds.x()/3.f, bounds.y()/3.f, bounds.z()/6.f });
                float meshScale = !mCaster.getClass().isActor() ? mCaster.getCellRef().getScale() : 1.0f;
                osg::Vec3f pos (mCaster.getRefData().getPosition().asVec3());
                MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + castStatic->mModel, effect->mParticle, pos, scale * meshScale);
            }

            if (animation && !mCaster.getClass().isActor())
                animation->addSpellCastGlow(effect);

            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };

            addedEffects.push_back("meshes\\" + castStatic->mModel);

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            if(!effect->mCastSound.empty())
                sndMgr->playSound3D(mCaster, effect->mCastSound, 1.0f, 1.0f);
            else
                sndMgr->playSound3D(mCaster, schools[effect->mData.mSchool]+" cast", 1.0f, 1.0f);
        }
    }

    bool CastSpell::spellIncreasesSkill()
    {
        return !mManualSpell && MWMechanics::spellIncreasesSkill(mId);
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
            MWWorld::ContainerStoreIterator item = inv.getSlot(slot);

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
                charge -= std::min(static_cast<int>(disintegrate), charge);
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

    void adjustDynamicStat(CreatureStats& creatureStats, int index, float magnitude, bool allowDecreaseBelowZero = false)
    {
        DynamicStat<float> stat = creatureStats.getDynamic(index);
        stat.setCurrent(stat.getCurrent() + magnitude, allowDecreaseBelowZero);
        creatureStats.setDynamic(index, stat);
    }

    bool effectTick(CreatureStats& creatureStats, const MWWorld::Ptr& actor, const EffectKey &effectKey, float magnitude)
    {
        if (magnitude == 0.f)
            return false;

        bool receivedMagicDamage = false;

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
            receivedMagicDamage = true;
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::DamageHealth, -magnitude);
            break;

        case ESM::MagicEffect::DamageMagicka:
        case ESM::MagicEffect::DamageFatigue:
        {
            int index = effectKey.mId-ESM::MagicEffect::DamageHealth;
            static const bool uncappedDamageFatigue = Settings::Manager::getBool("uncapped damage fatigue", "Game");
            adjustDynamicStat(creatureStats, index, -magnitude, index == 2 && uncappedDamageFatigue);
            break;
        }
        case ESM::MagicEffect::AbsorbHealth:
            if (magnitude > 0.f)
                receivedMagicDamage = true;
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);

            break;

        case ESM::MagicEffect::AbsorbMagicka:
        case ESM::MagicEffect::AbsorbFatigue:
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);
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
                        "fMagicSunBlockedMult")->mValue.getFloat();

            int weather = MWBase::Environment::get().getWorld()->getCurrentWeather();
            if (weather > 1)
                damageScale *= fMagicSunBlockedMult;

            adjustDynamicStat(creatureStats, 0, -magnitude * damageScale);
            if (magnitude * damageScale > 0.f)
                receivedMagicDamage = true;

            break;
        }

        case ESM::MagicEffect::FireDamage:
        case ESM::MagicEffect::ShockDamage:
        case ESM::MagicEffect::FrostDamage:
        case ESM::MagicEffect::Poison:
        {
            adjustDynamicStat(creatureStats, 0, -magnitude);
            receivedMagicDamage = true;
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
        static const std::map<int, std::string> summonMap
        {
            {ESM::MagicEffect::SummonAncestralGhost, "sMagicAncestralGhostID"},
            {ESM::MagicEffect::SummonBonelord, "sMagicBonelordID"},
            {ESM::MagicEffect::SummonBonewalker, "sMagicLeastBonewalkerID"},
            {ESM::MagicEffect::SummonCenturionSphere, "sMagicCenturionSphereID"},
            {ESM::MagicEffect::SummonClannfear, "sMagicClannfearID"},
            {ESM::MagicEffect::SummonDaedroth, "sMagicDaedrothID"},
            {ESM::MagicEffect::SummonDremora, "sMagicDremoraID"},
            {ESM::MagicEffect::SummonFabricant, "sMagicFabricantID"},
            {ESM::MagicEffect::SummonFlameAtronach, "sMagicFlameAtronachID"},
            {ESM::MagicEffect::SummonFrostAtronach, "sMagicFrostAtronachID"},
            {ESM::MagicEffect::SummonGoldenSaint, "sMagicGoldenSaintID"},
            {ESM::MagicEffect::SummonGreaterBonewalker, "sMagicGreaterBonewalkerID"},
            {ESM::MagicEffect::SummonHunger, "sMagicHungerID"},
            {ESM::MagicEffect::SummonScamp, "sMagicScampID"},
            {ESM::MagicEffect::SummonSkeletalMinion, "sMagicSkeletalMinionID"},
            {ESM::MagicEffect::SummonStormAtronach, "sMagicStormAtronachID"},
            {ESM::MagicEffect::SummonWingedTwilight, "sMagicWingedTwilightID"},
            {ESM::MagicEffect::SummonWolf, "sMagicCreature01ID"},
            {ESM::MagicEffect::SummonBear, "sMagicCreature02ID"},
            {ESM::MagicEffect::SummonBonewolf, "sMagicCreature03ID"},
            {ESM::MagicEffect::SummonCreature04, "sMagicCreature04ID"},
            {ESM::MagicEffect::SummonCreature05, "sMagicCreature05ID"}
        };

        auto it = summonMap.find(effectId);
        if (it != summonMap.end())
            return MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(it->second)->mValue.getString();
        return std::string();
    }

    void ApplyLoopingParticlesVisitor::visit (MWMechanics::EffectKey key,
                        const std::string& /*sourceName*/, const std::string& /*sourceId*/, int /*casterActorId*/,
                        float /*magnitude*/, float /*remainingTime*/, float /*totalTime*/)
    {
        const auto magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(key.mId);
        if ((magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) == 0)
            return;
        const ESM::Static* castStatic;
        if (!magicEffect->mHit.empty())
            castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
        else
            castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_DefaultHit");
        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mActor);
        if (anim && !castStatic->mModel.empty())
            anim->addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, /*loop*/true, "", magicEffect->mParticle);
    }
}
