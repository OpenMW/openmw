#include "spellcasting.hpp"

#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/animation.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "spelleffects.hpp"
#include "spellutil.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
    CastSpell::CastSpell(
        const MWWorld::Ptr& caster, const MWWorld::Ptr& target, const bool fromProjectile, const bool scriptedSpell)
        : mCaster(caster)
        , mTarget(target)
        , mFromProjectile(fromProjectile)
        , mScriptedSpell(scriptedSpell)
    {
    }

    void CastSpell::explodeSpell(
        const ESM::EffectList& effects, const MWWorld::Ptr& ignore, ESM::RangeType rangeType) const
    {
        const auto world = MWBase::Environment::get().getWorld();
        std::map<MWWorld::Ptr, std::vector<ESM::IndexedENAMstruct>> toApply;
        for (const ESM::IndexedENAMstruct& effectInfo : effects.mList)
        {
            const ESM::MagicEffect* effect = world->getStore().get<ESM::MagicEffect>().find(effectInfo.mData.mEffectID);

            if (effectInfo.mData.mRange != rangeType
                || (effectInfo.mData.mArea <= 0 && !ignore.isEmpty() && ignore.getClass().isActor()))
                continue; // Not right range type, or not area effect and hit an actor

            if (mFromProjectile && effectInfo.mData.mArea <= 0)
                continue; // Don't play explosion for projectiles with 0-area effects

            if (!mFromProjectile && effectInfo.mData.mRange == ESM::RT_Touch && !ignore.isEmpty()
                && !ignore.getClass().isActor() && !ignore.getClass().hasToolTip(ignore)
                && (mCaster.isEmpty() || mCaster.getClass().isActor()))
                continue; // Don't play explosion for touch spells on non-activatable objects except when spell is from
                          // a projectile enchantment or ExplodeSpell

            // Spawn the explosion orb effect
            const ESM::Static* areaStatic;
            if (!effect->mArea.empty())
                areaStatic = world->getStore().get<ESM::Static>().find(effect->mArea);
            else
                areaStatic = world->getStore().get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_DefaultArea"));

            const std::string& texture = effect->mParticle;

            if (effectInfo.mData.mArea <= 0)
            {
                if (effectInfo.mData.mRange == ESM::RT_Target)
                    world->spawnEffect(
                        Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(areaStatic->mModel)), texture,
                        mHitPosition, 1.0f);
                continue;
            }
            else
                world->spawnEffect(Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(areaStatic->mModel)),
                    texture, mHitPosition, static_cast<float>(effectInfo.mData.mArea * 2));

            // Play explosion sound (make sure to use NoTrack, since we will delete the projectile now)
            {
                MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
                if (!effect->mAreaSound.empty())
                    sndMgr->playSound3D(mHitPosition, effect->mAreaSound, 1.0f, 1.0f);
                else
                    sndMgr->playSound3D(mHitPosition,
                        world->getStore().get<ESM::Skill>().find(effect->mData.mSchool)->mSchool->mAreaSound, 1.0f,
                        1.0f);
            }
            // Get the actors in range of the effect
            std::vector<MWWorld::Ptr> objects;
            static const int unitsPerFoot = ceil(Constants::UnitsPerFoot);
            MWBase::Environment::get().getMechanicsManager()->getObjectsInRange(
                mHitPosition, static_cast<float>(effectInfo.mData.mArea * unitsPerFoot), objects);
            for (const MWWorld::Ptr& affected : objects)
            {
                // Ignore actors without collisions here, otherwise it will be possible to hit actors outside processing
                // range.
                if (affected.getClass().isActor() && !world->isActorCollisionEnabled(affected))
                    continue;

                auto& list = toApply[affected];
                list.push_back(effectInfo);
            }
        }

        // Now apply the appropriate effects to each actor in range
        for (auto& applyPair : toApply)
        {
            // Vanilla-compatible behaviour of never applying the spell to the caster
            // (could be changed by mods later)
            if (applyPair.first == mCaster)
                continue;

            if (applyPair.first == ignore)
                continue;

            ESM::EffectList effectsToApply;
            effectsToApply.mList = applyPair.second;
            inflict(applyPair.first, effectsToApply, rangeType, true);
        }
    }

    void CastSpell::launchMagicBolt() const
    {
        osg::Vec3f fallbackDirection(0, 1, 0);
        osg::Vec3f offset(0, 0, 0);
        if (!mTarget.isEmpty() && mTarget.getClass().isActor())
            offset.z() = MWBase::Environment::get().getWorld()->getHalfExtents(mTarget).z();

        // Fall back to a "caster to target" direction if we have no other means of determining it
        // (e.g. when cast by a non-actor)
        if (!mTarget.isEmpty())
            fallbackDirection = (mTarget.getRefData().getPosition().asVec3() + offset)
                - (mCaster.getRefData().getPosition().asVec3());

        MWBase::Environment::get().getWorld()->launchMagicBolt(mId, mCaster, fallbackDirection, mItem);
    }

    void CastSpell::inflict(
        const MWWorld::Ptr& target, const ESM::EffectList& effects, ESM::RangeType range, bool exploded) const
    {
        bool targetIsDeadActor = false;
        const bool targetIsActor = !target.isEmpty() && target.getClass().isActor();
        if (targetIsActor)
        {
            const auto& stats = target.getClass().getCreatureStats(target);
            if (stats.isDead() && stats.isDeathAnimationFinished())
                targetIsDeadActor = true;
        }

        // If none of the effects need to apply, we can early-out
        bool found = false;
        bool containsRecastable = false;
        const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>();
        for (const ESM::IndexedENAMstruct& effect : effects.mList)
        {
            if (effect.mData.mRange == range)
            {
                found = true;
                const ESM::MagicEffect* magicEffect = store.find(effect.mData.mEffectID);
                if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NonRecastable))
                    containsRecastable = true;
            }
        }
        if (!found)
            return;

        ActiveSpells::ActiveSpellParams params(mCaster, mId, mSourceName, mItem);
        params.setFlag(mFlags);
        bool castByPlayer = (!mCaster.isEmpty() && mCaster == getPlayer());

        const ActiveSpells* targetSpells = nullptr;
        if (targetIsActor)
            targetSpells = &target.getClass().getCreatureStats(target).getActiveSpells();

        // Re-casting a bound equipment effect has no effect if the spell is still active
        if (!containsRecastable && targetSpells && targetSpells->isSpellActive(mId))
        {
            if (castByPlayer)
                MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicCannotRecast}");
            return;
        }

        for (auto& enam : effects.mList)
        {
            if (target.isEmpty())
                break;

            if (enam.mData.mRange != range)
                continue;
            const ESM::MagicEffect* magicEffect = store.find(enam.mData.mEffectID);
            if (!magicEffect)
                continue;
            // caster needs to be an actor for linked effects (e.g. Absorb)
            if (magicEffect->mData.mFlags & ESM::MagicEffect::CasterLinked
                && (mCaster.isEmpty() || !mCaster.getClass().isActor()))
                continue;

            ActiveSpells::ActiveEffect effect;
            effect.mEffectId = enam.mData.mEffectID;
            effect.mArg = MWMechanics::EffectKey(enam.mData).mArg;
            effect.mMagnitude = 0.f;
            effect.mMinMagnitude = enam.mData.mMagnMin;
            effect.mMaxMagnitude = enam.mData.mMagnMax;
            effect.mTimeLeft = 0.f;
            effect.mEffectIndex = enam.mIndex;
            effect.mFlags = ESM::ActiveEffect::Flag_None;
            if (mScriptedSpell)
                effect.mFlags |= ESM::ActiveEffect::Flag_Ignore_Reflect;

            bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
            effect.mDuration = hasDuration ? static_cast<float>(enam.mData.mDuration) : 1.f;

            effect.mTimeLeft = effect.mDuration;

            // add to list of active effects, to apply in next frame
            params.getEffects().emplace_back(effect);

            bool effectAffectsHealth = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful
                || enam.mData.mEffectID == ESM::MagicEffect::RestoreHealth;
            if (castByPlayer && target != mCaster && targetIsActor && !targetIsDeadActor && effectAffectsHealth)
            {
                // If player is attempting to cast a harmful spell on or is healing a living target, show the target's
                // HP bar.
                MWBase::Environment::get().getWindowManager()->setEnemy(target);
            }

            if (!targetIsActor && magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
            {
                playEffects(target, *magicEffect);
            }
        }

        if (!exploded)
            explodeSpell(effects, target, range);

        if (!target.isEmpty())
        {
            if (!params.getEffects().empty())
            {
                if (targetIsActor)
                {
                    if (!targetIsDeadActor)
                        target.getClass().getCreatureStats(target).getActiveSpells().addSpell(params);
                }
                else
                {
                    // Apply effects instantly. We can ignore effect deletion since the entire params object gets
                    // deleted afterwards anyway and we can ignore reflection since non-actors cannot reflect spells
                    for (auto& effect : params.getEffects())
                        applyMagicEffect(target, mCaster, params, effect, 0.f);
                }
            }
        }
    }

    bool CastSpell::cast(const ESM::RefId& id)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        if (const auto spell = store.get<ESM::Spell>().search(id))
            return cast(spell);

        if (const auto potion = store.get<ESM::Potion>().search(id))
            return cast(potion);

        if (const auto ingredient = store.get<ESM::Ingredient>().search(id))
            return cast(ingredient);

        throw std::runtime_error("ID type cannot be casted");
    }

    bool CastSpell::cast(const MWWorld::Ptr& item, bool launchProjectile)
    {
        const ESM::RefId& enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            throw std::runtime_error("can't cast an item without an enchantment");

        mSourceName = item.getClass().getName(item);
        mId = item.getCellRef().getRefId();

        const auto& store = MWBase::Environment::get().getESMStore();
        const ESM::Enchantment* enchantment = store->get<ESM::Enchantment>().find(enchantmentName);

        // CastOnce enchantments (i.e. scrolls) never stack and the item is immediately destroyed,
        // so don't track the source item.
        if (enchantment->mData.mType != ESM::Enchantment::CastOnce)
            mItem = item.getCellRef().getRefNum();

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
        if (!godmode
            && (type == ESM::Enchantment::WhenUsed || (!isProjectile && type == ESM::Enchantment::WhenStrikes)))
        {
            int castCost = getEffectiveEnchantmentCastCost(*enchantment, mCaster);

            if (item.getCellRef().getEnchantmentCharge() == -1)
                item.getCellRef().setEnchantmentCharge(
                    static_cast<float>(MWMechanics::getEnchantmentCharge(*enchantment)));

            if (item.getCellRef().getEnchantmentCharge() < castCost)
            {
                if (mCaster == getPlayer())
                {
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInsufficientCharge}");

                    // Failure sound
                    ESM::RefId school = ESM::Skill::Alteration;
                    if (!enchantment->mEffects.mList.empty())
                    {
                        short effectId = enchantment->mEffects.mList.front().mData.mEffectID;
                        const ESM::MagicEffect* magicEffect = store->get<ESM::MagicEffect>().find(effectId);
                        school = magicEffect->mData.mSchool;
                    }

                    MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
                    sndMgr->playSound3D(
                        mCaster, store->get<ESM::Skill>().find(school)->mSchool->mFailureSound, 1.0f, 1.0f);
                }
                return false;
            }
            // Reduce charge
            item.getCellRef().setEnchantmentCharge(item.getCellRef().getEnchantmentCharge() - castCost);
        }

        if (type == ESM::Enchantment::WhenUsed)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded(mCaster, ESM::Skill::Enchant, ESM::Skill::Enchant_UseMagicItem);
        }
        else if (type == ESM::Enchantment::CastOnce)
        {
            if (!godmode)
                item.getContainerStore()->remove(item, 1);
        }
        else if (type == ESM::Enchantment::WhenStrikes)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded(mCaster, ESM::Skill::Enchant, ESM::Skill::Enchant_CastOnStrike);
        }

        if (isProjectile)
            inflict(mTarget, enchantment->mEffects, ESM::RT_Self);
        else
            inflict(mCaster, enchantment->mEffects, ESM::RT_Self);

        if (isProjectile || !mTarget.isEmpty())
            inflict(mTarget, enchantment->mEffects, ESM::RT_Touch);

        if (launchProjectile)
            launchMagicBolt();
        else if (isProjectile || !mTarget.isEmpty())
            inflict(mTarget, enchantment->mEffects, ESM::RT_Target);

        return true;
    }

    bool CastSpell::cast(const ESM::Potion* potion)
    {
        mSourceName = potion->mName;
        mId = potion->mId;
        mFlags = static_cast<ESM::ActiveSpells::Flags>(
            ESM::ActiveSpells::Flag_Temporary | ESM::ActiveSpells::Flag_Stackable);

        // Ignore range and don't apply area of effect
        inflict(mCaster, potion->mEffects, ESM::RT_Self, true);
        inflict(mCaster, potion->mEffects, ESM::RT_Touch, true);
        inflict(mCaster, potion->mEffects, ESM::RT_Target, true);

        return true;
    }

    bool CastSpell::cast(const ESM::Spell* spell)
    {
        mSourceName = spell->mName;
        mId = spell->mId;

        ESM::RefId school = ESM::Skill::Alteration;

        bool godmode = mCaster == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        if (mCaster.getClass().isActor() && !mAlwaysSucceed && !mScriptedSpell)
        {
            school = getSpellSchool(spell, mCaster);

            CreatureStats& stats = mCaster.getClass().getCreatureStats(mCaster);

            if (!godmode)
            {
                bool fail = false;

                // Check success
                float successChance = getSpellSuccessChance(spell, mCaster, nullptr, true, false);
                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                if (Misc::Rng::roll0to99(prng) >= successChance)
                {
                    if (mCaster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicSkillFail}");
                    fail = true;
                }

                if (fail)
                {
                    // Failure sound
                    MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
                    const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(school);
                    sndMgr->playSound3D(mCaster, skill->mSchool->mFailureSound, 1.0f, 1.0f);
                    return false;
                }
            }

            // A power can be used once per 24h
            if (spell->mData.mType == ESM::Spell::ST_Power)
                stats.getSpells().usePower(spell);
        }

        if (!mScriptedSpell && mCaster == getPlayer() && spellIncreasesSkill(spell))
            mCaster.getClass().skillUsageSucceeded(mCaster, school, ESM::Skill::Spellcast_Success);

        // A non-actor doesn't play its spell cast effects from a character controller, so play them here
        if (!mCaster.getClass().isActor())
            playSpellCastingEffects(spell->mEffects.mList);

        inflict(mCaster, spell->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
            inflict(mTarget, spell->mEffects, ESM::RT_Touch);

        launchMagicBolt();

        return true;
    }

    bool CastSpell::cast(const ESM::Ingredient* ingredient)
    {
        mId = ingredient->mId;
        mFlags = static_cast<ESM::ActiveSpells::Flags>(
            ESM::ActiveSpells::Flag_Temporary | ESM::ActiveSpells::Flag_Stackable);
        mSourceName = ingredient->mName;

        std::optional<ESM::EffectList> effect = rollIngredientEffect(mCaster, ingredient, 0);

        if (effect)
        {
            inflict(mCaster, *effect, ESM::RT_Self);
            return true;
        }

        if (mCaster == getPlayer())
        {
            // "X has no effect on you"
            std::string message = MWBase::Environment::get()
                                      .getESMStore()
                                      ->get<ESM::GameSetting>()
                                      .find("sNotifyMessage50")
                                      ->mValue.getString();
            message = Misc::StringUtils::format(message, ingredient->mName);
            MWBase::Environment::get().getWindowManager()->messageBox(message);
        }

        return false;
    }

    void CastSpell::playSpellCastingEffects(const ESM::Enchantment* enchantment) const
    {
        playSpellCastingEffects(enchantment->mEffects.mList);
    }

    void CastSpell::playSpellCastingEffects(const ESM::Spell* spell) const
    {
        playSpellCastingEffects(spell->mEffects.mList);
    }

    void CastSpell::playSpellCastingEffects(const std::vector<ESM::IndexedENAMstruct>& effects) const
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        std::vector<VFS::Path::Normalized> addedEffects;

        for (const ESM::IndexedENAMstruct& effectData : effects)
        {
            const auto effect = store.get<ESM::MagicEffect>().find(effectData.mData.mEffectID);

            const ESM::Static* castStatic;

            if (!effect->mCasting.empty())
                castStatic = store.get<ESM::Static>().find(effect->mCasting);
            else
                castStatic = store.get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_DefaultCast"));

            VFS::Path::Normalized castStaticModel
                = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(castStatic->mModel));

            // check if the effect was already added
            if (std::find(addedEffects.begin(), addedEffects.end(), castStaticModel) != addedEffects.end())
                continue;

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(mCaster);
            if (animation)
            {
                animation->addEffect(castStaticModel.value(), ESM::MagicEffect::indexToName(effect->mIndex), false, {},
                    effect->mParticle);
            }
            else
            {
                // If the caster has no animation, add the effect directly to the effectManager
                // We must scale and position it manually
                float scale = mCaster.getCellRef().getScale();
                osg::Vec3f pos(mCaster.getRefData().getPosition().asVec3());
                if (!mCaster.getClass().isNpc())
                {
                    osg::Vec3f bounds(MWBase::Environment::get().getWorld()->getHalfExtents(mCaster) * 2.f);
                    scale *= std::max({ bounds.x(), bounds.y(), bounds.z() / 2.f }) / 64.f;
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
                    osg::Vec3f npcScaleVec(1.f, 1.f, 1.f);
                    mCaster.getClass().adjustScale(mCaster, npcScaleVec, true);
                    scale *= npcScaleVec.z();
                }
                scale = std::max(scale, 1.f);
                MWBase::Environment::get().getWorld()->spawnEffect(castStaticModel, effect->mParticle, pos, scale);
            }

            if (animation && !mCaster.getClass().isActor())
                animation->addSpellCastGlow(effect->getColor());

            addedEffects.push_back(std::move(castStaticModel));

            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
            if (!effect->mCastSound.empty())
                sndMgr->playSound3D(mCaster, effect->mCastSound, 1.0f, 1.0f);
            else
                sndMgr->playSound3D(
                    mCaster, store.get<ESM::Skill>().find(effect->mData.mSchool)->mSchool->mCastSound, 1.0f, 1.0f);
        }
    }

    void playEffects(const MWWorld::Ptr& target, const ESM::MagicEffect& magicEffect, bool playNonLooping)
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        if (playNonLooping)
        {
            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
            if (!magicEffect.mHitSound.empty())
                sndMgr->playSound3D(target, magicEffect.mHitSound, 1.0f, 1.0f);
            else
                sndMgr->playSound3D(
                    target, store->get<ESM::Skill>().find(magicEffect.mData.mSchool)->mSchool->mHitSound, 1.0f, 1.0f);
        }

        // Add VFX
        const ESM::Static* castStatic;
        if (!magicEffect.mHit.empty())
            castStatic = store->get<ESM::Static>().find(magicEffect.mHit);
        else
            castStatic = store->get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_DefaultHit"));

        bool loop = (magicEffect.mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
        if (anim && !castStatic->mModel.empty())
        {
            // Don't play particle VFX unless the effect is new or it should be looping.
            if (playNonLooping || loop)
            {
                const VFS::Path::Normalized castStaticModel
                    = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(castStatic->mModel));
                anim->addEffect(castStaticModel.value(), ESM::MagicEffect::indexToName(magicEffect.mIndex), loop, {},
                    magicEffect.mParticle);
            }
        }
    }
}
