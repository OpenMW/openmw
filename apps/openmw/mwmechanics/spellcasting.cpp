#include "spellcasting.hpp"

#include <components/misc/constants.hpp>
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

#include "actorutil.hpp"
#include "aifollow.hpp"
#include "creaturestats.hpp"
#include "spelleffects.hpp"
#include "spellutil.hpp"
#include "summoning.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
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

        MWBase::Environment::get().getWorld()->launchMagicBolt(mId, mCaster, fallbackDirection, mSlot);
    }

    void CastSpell::inflict(const MWWorld::Ptr &target, const MWWorld::Ptr &caster,
                            const ESM::EffectList &effects, ESM::RangeType range, bool exploded)
    {
        const bool targetIsActor = !target.isEmpty() && target.getClass().isActor();
        if (targetIsActor)
        {
            // Early-out for characters that have departed.
            const auto& stats = target.getClass().getCreatureStats(target);
            if (stats.isDead() && stats.isDeathAnimationFinished())
                return;
        }

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
        if (spell && targetIsActor && (spell->mData.mType == ESM::Spell::ST_Disease || spell->mData.mType == ESM::Spell::ST_Blight))
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

        ActiveSpells::ActiveSpellParams params(*this, caster);

        bool castByPlayer = (!caster.isEmpty() && caster == getPlayer());

        const ActiveSpells* targetSpells = nullptr;
        if (targetIsActor)
            targetSpells = &target.getClass().getCreatureStats(target).getActiveSpells();

        bool canCastAnEffect = false;    // For bound equipment.If this remains false
                                         // throughout the iteration of this spell's 
                                         // effects, we display a "can't re-cast" message

        int currentEffectIndex = 0;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (effects.mList.begin());
             !target.isEmpty() && effectIt != effects.mList.end(); ++effectIt, ++currentEffectIndex)
        {
            if (effectIt->mRange != range)
                continue;

            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                effectIt->mEffectID);

            // Re-casting a bound equipment effect has no effect if the spell is still active
            if (magicEffect->mData.mFlags & ESM::MagicEffect::NonRecastable && targetSpells && targetSpells->isSpellActive(mId))
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

            ActiveSpells::ActiveEffect effect;
            effect.mEffectId = effectIt->mEffectID;
            effect.mArg = MWMechanics::EffectKey(*effectIt).mArg;
            effect.mMagnitude = 0.f;
            effect.mMinMagnitude = effectIt->mMagnMin;
            effect.mMaxMagnitude = effectIt->mMagnMax;
            effect.mTimeLeft = 0.f;
            effect.mEffectIndex = currentEffectIndex;
            effect.mFlags = ESM::ActiveEffect::Flag_None;
            if(mManualSpell)
                effect.mFlags |= ESM::ActiveEffect::Flag_Ignore_Reflect;

            bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
            effect.mDuration = hasDuration ? static_cast<float>(effectIt->mDuration) : 1.f;

            bool appliedOnce = magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce;
            if (!appliedOnce)
                effect.mDuration = std::max(1.f, effect.mDuration);

            effect.mTimeLeft = effect.mDuration;

            // add to list of active effects, to apply in next frame
            params.getEffects().emplace_back(effect);

            bool effectAffectsHealth = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful || effectIt->mEffectID == ESM::MagicEffect::RestoreHealth;
            if (castByPlayer && target != caster && targetIsActor && effectAffectsHealth)
            {
                // If player is attempting to cast a harmful spell on or is healing a living target, show the target's HP bar.
                MWBase::Environment::get().getWindowManager()->setEnemy(target);
            }

            if (!targetIsActor && magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
            {
                playEffects(target, *magicEffect);
            }
        }

        if (!exploded)
            MWBase::Environment::get().getWorld()->explodeSpell(mHitPosition, effects, caster, target, range, mId, mSourceName, mFromProjectile, mSlot);

        if (!target.isEmpty())
        {
            if (!params.getEffects().empty())
            {
                if(targetIsActor)
                    target.getClass().getCreatureStats(target).getActiveSpells().addSpell(params);
                else
                {
                    // Apply effects instantly. We can ignore effect deletion since the entire params object gets deleted afterwards anyway
                    // and we can ignore reflection since non-actors cannot reflect spells
                    for(auto& effect : params.getEffects())
                        applyMagicEffect(target, caster, params, effect, 0.f);
                }
            }
        }
    }

    bool CastSpell::cast(const std::string &id)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        if (const auto spell = store.get<ESM::Spell>().search(id))
            return cast(spell);

        if (const auto potion = store.get<ESM::Potion>().search(id))
            return cast(potion);

        if (const auto ingredient = store.get<ESM::Ingredient>().search(id))
            return cast(ingredient);

        throw std::runtime_error("ID type cannot be casted");
    }

    bool CastSpell::cast(const MWWorld::Ptr &item, int slot, bool launchProjectile)
    {
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            throw std::runtime_error("can't cast an item without an enchantment");

        mSourceName = item.getClass().getName(item);
        mId = item.getCellRef().getRefId();

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);

        mSlot = slot;

        bool godmode = mCaster == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();
        bool isProjectile = false;
        if (item.getType() == ESM::Weapon::sRecordId)
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

        if (isProjectile)
            inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Self);
        else
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
        mType = ESM::ActiveSpells::Type_Consumable;

        inflict(mCaster, mCaster, potion->mEffects, ESM::RT_Self);

        return true;
    }

    bool CastSpell::cast(const ESM::Spell* spell)
    {
        mSourceName = spell->mName;
        mId = spell->mId;

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

                float fatigueLoss = MWMechanics::calcSpellCost(*spell) * (fFatigueSpellBase + normalizedEncumbrance * fFatigueSpellMult);
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

        if (!mManualSpell && mCaster == getPlayer() && spellIncreasesSkill(spell))
            mCaster.getClass().skillUsageSucceeded(mCaster, spellSchoolToSkill(school), 0);

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
        mType = ESM::ActiveSpells::Type_Consumable;
        mSourceName = ingredient->mName;

        ESM::ENAMstruct effect;
        effect.mEffectID = ingredient->mData.mEffectID[0];
        effect.mSkill = ingredient->mData.mSkills[0];
        effect.mAttribute = ingredient->mData.mAttributes[0];
        effect.mRange = ESM::RT_Self;
        effect.mArea = 0;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const auto magicEffect = store.get<ESM::MagicEffect>().find(effect.mEffectID);
        const MWMechanics::CreatureStats& creatureStats = mCaster.getClass().getCreatureStats(mCaster);

        float x = (mCaster.getClass().getSkill(mCaster, ESM::Skill::Alchemy) +
                    0.2f * creatureStats.getAttribute (ESM::Attribute::Intelligence).getModified()
                    + 0.1f * creatureStats.getAttribute (ESM::Attribute::Luck).getModified())
                    * creatureStats.getFatigueTerm();

        int roll = Misc::Rng::roll0to99();
        if (roll > x)
        {
            // "X has no effect on you"
            std::string message = store.get<ESM::GameSetting>().find("sNotifyMessage50")->mValue.getString();
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
                // We must scale and position it manually
                float scale = mCaster.getCellRef().getScale();
                osg::Vec3f pos (mCaster.getRefData().getPosition().asVec3());
                if (!mCaster.getClass().isNpc())
                {
                    osg::Vec3f bounds (MWBase::Environment::get().getWorld()->getHalfExtents(mCaster) * 2.f);
                    scale *= std::max({bounds.x(), bounds.y(), bounds.z() / 2.f}) / 64.f;
                    float offset = 0.f;
                    if (bounds.z() < 128.f)
                        offset = bounds.z() - 128.f;
                    else if (bounds.z() < bounds.x() + bounds.y())
                        offset = 128.f - bounds.z();
                    if (MWBase::Environment::get().getWorld()->isFlying(mCaster))
                        offset /= 20.f;
                    pos.z() += offset * scale;
                }
                else
                {
                    // Additionally use the NPC's height
                    osg::Vec3f npcScaleVec (1.f, 1.f, 1.f);
                    mCaster.getClass().adjustScale(mCaster, npcScaleVec, true);
                    scale *= npcScaleVec.z();
                }
                scale = std::max(scale, 1.f);
                MWBase::Environment::get().getWorld()->spawnEffect("meshes\\" + castStatic->mModel, effect->mParticle, pos, scale);
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

    void playEffects(const MWWorld::Ptr& target, const ESM::MagicEffect& magicEffect, bool playNonLooping)
    {
        if (playNonLooping)
        {
            static const std::string schools[] = {
                "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
            };

            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            if(!magicEffect.mHitSound.empty())
                sndMgr->playSound3D(target, magicEffect.mHitSound, 1.0f, 1.0f);
            else
                sndMgr->playSound3D(target, schools[magicEffect.mData.mSchool]+" hit", 1.0f, 1.0f);
        }

        // Add VFX
        const ESM::Static* castStatic;
        if (!magicEffect.mHit.empty())
            castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect.mHit);
        else
            castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_DefaultHit");

        bool loop = (magicEffect.mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
        if(anim && !castStatic->mModel.empty())
        {
            // Don't play particle VFX unless the effect is new or it should be looping.
            if (playNonLooping || loop)
                anim->addEffect("meshes\\" + castStatic->mModel, magicEffect.mIndex, loop, "", magicEffect.mParticle);
        }
    }
}
