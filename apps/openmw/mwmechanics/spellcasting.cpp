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
        const MWWorld::Ptr& caster, const MWWorld::Ptr& target, const bool fromProjectile, const bool manualSpell)
        : mCaster(caster)
        , mTarget(target)
        , mFromProjectile(fromProjectile)
        , mManualSpell(manualSpell)
    {
    }

    void CastSpell::explodeSpell(
        const ESM::EffectList& effects, const MWWorld::Ptr& ignore, ESM::RangeType rangeType) const
    {
        const auto world = MWBase::Environment::get().getWorld();
        std::map<MWWorld::Ptr, std::vector<ESM::ENAMstruct>> toApply;
        int index = -1;
        for (const ESM::ENAMstruct& effectInfo : effects.mList)
        {
            ++index;
            const ESM::MagicEffect* effect = world->getStore().get<ESM::MagicEffect>().find(effectInfo.mEffectID);

            if (effectInfo.mRange != rangeType
                || (effectInfo.mArea <= 0 && !ignore.isEmpty() && ignore.getClass().isActor()))
                continue; // Not right range type, or not area effect and hit an actor

            if (mFromProjectile && effectInfo.mArea <= 0)
                continue; // Don't play explosion for projectiles with 0-area effects

            if (!mFromProjectile && effectInfo.mRange == ESM::RT_Touch && !ignore.isEmpty()
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

            if (effectInfo.mArea <= 0)
            {
                if (effectInfo.mRange == ESM::RT_Target)
                    world->spawnEffect(
                        Misc::ResourceHelpers::correctMeshPath(areaStatic->mModel), texture, mHitPosition, 1.0f);
                continue;
            }
            else
                world->spawnEffect(Misc::ResourceHelpers::correctMeshPath(areaStatic->mModel), texture, mHitPosition,
                    static_cast<float>(effectInfo.mArea * 2));

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
                mHitPosition, static_cast<float>(effectInfo.mArea * unitsPerFoot), objects);
            for (const MWWorld::Ptr& affected : objects)
            {
                // Ignore actors without collisions here, otherwise it will be possible to hit actors outside processing
                // range.
                if (affected.getClass().isActor() && !world->isActorCollisionEnabled(affected))
                    continue;

                auto& list = toApply[affected];
                while (list.size() < static_cast<std::size_t>(index))
                {
                    // Insert dummy effects to preserve indices
                    auto& dummy = list.emplace_back(effectInfo);
                    dummy.mRange = ESM::RT_Self;
                    assert(dummy.mRange != rangeType);
                }
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
        bool containsRecastable = false;
        std::vector<const ESM::MagicEffect*> magicEffects;
        magicEffects.reserve(effects.mList.size());
        const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>();
        for (const ESM::ENAMstruct& effect : effects.mList)
        {
            if (effect.mRange == range)
            {
                found = true;
                const ESM::MagicEffect* magicEffect = store.find(effect.mEffectID);
                // caster needs to be an actor for linked effects (e.g. Absorb)
                if (magicEffect->mData.mFlags & ESM::MagicEffect::CasterLinked
                    && (mCaster.isEmpty() || !mCaster.getClass().isActor()))
                {
                    magicEffects.push_back(nullptr);
                    continue;
                }
                if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NonRecastable))
                    containsRecastable = true;
                magicEffects.push_back(magicEffect);
            }
            else
                magicEffects.push_back(nullptr);
        }
        if (!found)
            return;

        ActiveSpells::ActiveSpellParams params(*this, mCaster);
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

        for (size_t currentEffectIndex = 0; !target.isEmpty() && currentEffectIndex < effects.mList.size();
             ++currentEffectIndex)
        {
            const ESM::ENAMstruct& enam = effects.mList[currentEffectIndex];
            if (enam.mRange != range)
                continue;

            const ESM::MagicEffect* magicEffect = magicEffects[currentEffectIndex];
            if (!magicEffect)
                continue;

            ActiveSpells::ActiveEffect effect;
            effect.mEffectId = enam.mEffectID;
            effect.mArg = MWMechanics::EffectKey(enam).mArg;
            effect.mMagnitude = 0.f;
            effect.mMinMagnitude = enam.mMagnMin;
            effect.mMaxMagnitude = enam.mMagnMax;
            effect.mTimeLeft = 0.f;
            effect.mEffectIndex = static_cast<int>(currentEffectIndex);
            effect.mFlags = ESM::ActiveEffect::Flag_None;
            if (mManualSpell)
                effect.mFlags |= ESM::ActiveEffect::Flag_Ignore_Reflect;

            bool hasDuration = !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration);
            effect.mDuration = hasDuration ? static_cast<float>(enam.mDuration) : 1.f;

            bool appliedOnce = magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce;
            if (!appliedOnce)
                effect.mDuration = std::max(1.f, effect.mDuration);

            effect.mTimeLeft = effect.mDuration;

            // add to list of active effects, to apply in next frame
            params.getEffects().emplace_back(effect);

            bool effectAffectsHealth = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful
                || enam.mEffectID == ESM::MagicEffect::RestoreHealth;
            if (castByPlayer && target != mCaster && targetIsActor && effectAffectsHealth)
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
                    target.getClass().getCreatureStats(target).getActiveSpells().addSpell(params);
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
                        short effectId = enchantment->mEffects.mList.front().mEffectID;
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
                mCaster.getClass().skillUsageSucceeded(mCaster, ESM::Skill::Enchant, 1);
        }
        else if (type == ESM::Enchantment::CastOnce)
        {
            if (!godmode)
                item.getContainerStore()->remove(item, 1);
        }
        else if (type == ESM::Enchantment::WhenStrikes)
        {
            if (mCaster == getPlayer())
                mCaster.getClass().skillUsageSucceeded(mCaster, ESM::Skill::Enchant, 3);
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
        mType = ESM::ActiveSpells::Type_Consumable;

        inflict(mCaster, potion->mEffects, ESM::RT_Self);

        return true;
    }

    bool CastSpell::cast(const ESM::Spell* spell)
    {
        mSourceName = spell->mName;
        mId = spell->mId;

        ESM::RefId school = ESM::Skill::Alteration;

        bool godmode = mCaster == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        if (mCaster.getClass().isActor() && !mAlwaysSucceed && !mManualSpell)
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

        if (!mManualSpell && mCaster == getPlayer() && spellIncreasesSkill(spell))
            mCaster.getClass().skillUsageSucceeded(mCaster, school, 0);

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
        mType = ESM::ActiveSpells::Type_Consumable;
        mSourceName = ingredient->mName;

        ESM::ENAMstruct effect;
        effect.mEffectID = ingredient->mData.mEffectID[0];
        effect.mSkill = ingredient->mData.mSkills[0];
        effect.mAttribute = ingredient->mData.mAttributes[0];
        effect.mRange = ESM::RT_Self;
        effect.mArea = 0;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const auto magicEffect = store.get<ESM::MagicEffect>().find(effect.mEffectID);
        const MWMechanics::CreatureStats& creatureStats = mCaster.getClass().getCreatureStats(mCaster);

        float x = (mCaster.getClass().getSkill(mCaster, ESM::Skill::Alchemy)
                      + 0.2f * creatureStats.getAttribute(ESM::Attribute::Intelligence).getModified()
                      + 0.1f * creatureStats.getAttribute(ESM::Attribute::Luck).getModified())
            * creatureStats.getFatigueTerm();

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        int roll = Misc::Rng::roll0to99(prng);
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

        inflict(mCaster, effects, ESM::RT_Self);

        return true;
    }

    void CastSpell::playSpellCastingEffects(const ESM::Enchantment* enchantment) const
    {
        playSpellCastingEffects(enchantment->mEffects.mList);
    }

    void CastSpell::playSpellCastingEffects(const ESM::Spell* spell) const
    {
        playSpellCastingEffects(spell->mEffects.mList);
    }

    void CastSpell::playSpellCastingEffects(const std::vector<ESM::ENAMstruct>& effects) const
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        std::vector<std::string> addedEffects;

        for (const ESM::ENAMstruct& effectData : effects)
        {
            const auto effect = store.get<ESM::MagicEffect>().find(effectData.mEffectID);

            const ESM::Static* castStatic;

            if (!effect->mCasting.empty())
                castStatic = store.get<ESM::Static>().find(effect->mCasting);
            else
                castStatic = store.get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_DefaultCast"));

            // check if the effect was already added
            if (std::find(addedEffects.begin(), addedEffects.end(),
                    Misc::ResourceHelpers::correctMeshPath(castStatic->mModel))
                != addedEffects.end())
                continue;

            MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(mCaster);
            if (animation)
            {
                animation->addEffect(Misc::ResourceHelpers::correctMeshPath(castStatic->mModel),
                    ESM::MagicEffect::indexToName(effect->mIndex), false, {}, effect->mParticle);
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
                MWBase::Environment::get().getWorld()->spawnEffect(
                    Misc::ResourceHelpers::correctMeshPath(castStatic->mModel), effect->mParticle, pos, scale);
            }

            if (animation && !mCaster.getClass().isActor())
                animation->addSpellCastGlow(effect);

            addedEffects.push_back(Misc::ResourceHelpers::correctMeshPath(castStatic->mModel));

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
                anim->addEffect(Misc::ResourceHelpers::correctMeshPath(castStatic->mModel),
                    ESM::MagicEffect::indexToName(magicEffect.mIndex), loop, {}, magicEffect.mParticle);
        }
    }
}
