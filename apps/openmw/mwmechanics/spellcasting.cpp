#include "spellcasting.hpp"

#include <boost/format.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"


#include "../mwworld/containerstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwrender/animation.hpp"

namespace MWMechanics
{

    CastSpell::CastSpell(const MWWorld::Ptr &caster, const MWWorld::Ptr &target)
        : mCaster(caster)
        , mTarget(target)
        , mStack(false)
    {
    }

    void CastSpell::inflict(const MWWorld::Ptr &target, const MWWorld::Ptr &caster,
                            const ESM::EffectList &effects, ESM::RangeType range, bool reflected)
    {
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
        std::vector<ActiveSpells::Effect> appliedLastingEffects;
        bool firstAppliedEffect = true;

        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (effects.mList.begin());
            effectIt!=effects.mList.end(); ++effectIt)
        {
            if (effectIt->mRange != range)
                continue;

            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                effectIt->mEffectID);

            float magnitudeMult = 1;
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful && target.getClass().isActor())
            {
                // If player is attempting to cast a harmful spell, show the target's HP bar
                if (caster.getRefData().getHandle() == "player" && target != caster)
                    MWBase::Environment::get().getWindowManager()->setEnemy(target);

                // Try absorbing if it's a spell
                // NOTE: Vanilla does this once per effect source instead of adding the % from all sources together, not sure
                // if that is worth replicating.
                if (spell && caster != target)
                {
                    int absorb = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).mMagnitude;
                    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                    bool isAbsorbed = (roll < absorb);
                    if (isAbsorbed)
                    {
                        const ESM::Static* absorbStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Absorb");
                        MWBase::Environment::get().getWorld()->getAnimation(target)->addEffect(
                                    "meshes\\" + absorbStatic->mModel, ESM::MagicEffect::Reflect, false, "");
                        // Magicka is increased by cost of spell
                        DynamicStat<float> magicka = target.getClass().getCreatureStats(target).getMagicka();
                        magicka.setCurrent(magicka.getCurrent() + spell->mData.mCost);
                        target.getClass().getCreatureStats(target).setMagicka(magicka);
                        magnitudeMult = 0;
                    }
                }

                // Try reflecting
                if (!reflected && magnitudeMult > 0 && caster != target && !(magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable))
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

                    magnitudeMult = MWMechanics::getEffectMultiplier(effectIt->mEffectID, target, caster, spell);
                    if (magnitudeMult == 0)
                    {
                        // Fully resisted, show message
                        if (target.getRefData().getHandle() == "player")
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                        else
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResisted}");
                    }
                }
            }


            if (magnitudeMult > 0)
            {
                float random = std::rand() / static_cast<float>(RAND_MAX);
                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * random;
                magnitude *= magnitudeMult;                    

                if (target.getClass().isActor() && !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
                {
                    ActiveSpells::Effect effect;
                    effect.mKey = MWMechanics::EffectKey(*effectIt);
                    effect.mDuration = effectIt->mDuration;
                    effect.mMagnitude = magnitude;

                    appliedLastingEffects.push_back(effect);

                    // For absorb effects, also apply the effect to the caster - but with a negative
                    // magnitude, since we're transfering stats from the target to the caster
                    for (int i=0; i<5; ++i)
                    {
                        if (effectIt->mEffectID == ESM::MagicEffect::AbsorbAttribute+i)
                        {
                            std::vector<ActiveSpells::Effect> effects;
                            ActiveSpells::Effect effect_ = effect;
                            effect_.mMagnitude *= -1;
                            effects.push_back(effect_);
                            caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell("", true, effects, mSourceName);
                        }
                    }
                }
                else
                    applyInstantEffect(target, effectIt->mEffectID, magnitude);

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
                    if (!magicEffect->mHit.empty())
                    {
                        const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
                        bool loop = magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx;
                        // Note: in case of non actor, a free effect should be fine as well
                        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(target);
                        if (anim)
                            anim->addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "");
                    }
                }

                // TODO: For Area effects, launch a growing particle effect that applies the effect to more actors as it hits them. Best managed in World.
            }
        }

        if (reflectedEffects.mList.size())
            inflict(caster, target, reflectedEffects, range, true);

        if (appliedLastingEffects.size())
            target.getClass().getCreatureStats(target).getActiveSpells().addSpell(mId, mStack, appliedLastingEffects, mSourceName);
    }

    void CastSpell::applyInstantEffect(const MWWorld::Ptr &target, short effectId, float magnitude)
    {
        if (!target.getClass().isActor())
        {
            if (effectId == ESM::MagicEffect::Lock)
            {
                if (target.getCellRef().mLockLevel < magnitude)
                    target.getCellRef().mLockLevel = magnitude;
            }
            else if (effectId == ESM::MagicEffect::Open)
            {
                // TODO: This is a crime
                if (target.getCellRef().mLockLevel <= magnitude)
                {
                    if (target.getCellRef().mLockLevel > 0)
                        MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock", 1.f, 1.f);
                    target.getCellRef().mLockLevel = 0;
                }
                else
                    MWBase::Environment::get().getSoundManager()->playSound3D(target, "Open Lock Fail", 1.f, 1.f);
            }
        }
        else
        {
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
                target.getClass().getCreatureStats(target).getActiveSpells().purgeAll();
            else if (effectId == ESM::MagicEffect::RemoveCurse)
                target.getClass().getCreatureStats(target).getSpells().purgeCurses();

            if (target.getRefData().getHandle() != "player")
                return;
            if (!MWBase::Environment::get().getWorld()->isTeleportingEnabled())
                return;

            Ogre::Vector3 worldPos;
            if (!MWBase::Environment::get().getWorld()->findInteriorPositionInWorldSpace(target.getCell(), worldPos))
                worldPos = MWBase::Environment::get().getWorld()->getPlayer().getLastKnownExteriorPosition();

            if (effectId == ESM::MagicEffect::DivineIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "divinemarker",
                                                                               worldPos);
            }
            else if (effectId == ESM::MagicEffect::AlmsiviIntervention)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(target, "templemarker",
                                                                               worldPos);
            }

            else if (effectId == ESM::MagicEffect::Mark)
            {
                // TODO
            }
            else if (effectId == ESM::MagicEffect::Recall)
            {
                // TODO
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
        mId = item.getCellRef().mRefID;

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);

        mStack = (enchantment->mData.mType == ESM::Enchantment::CastOnce);

        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed)
        {
            // Check if there's enough charge left
            const float enchantCost = enchantment->mData.mCost;
            MWMechanics::NpcStats &stats = MWWorld::Class::get(mCaster).getNpcStats(mCaster);
            int eSkill = stats.getSkill(ESM::Skill::Enchant).getModified();
            const int castCost = std::max(1.f, enchantCost - (enchantCost / 100) * (eSkill - 10));

            if (item.getCellRef().mEnchantmentCharge == -1)
                item.getCellRef().mEnchantmentCharge = enchantment->mData.mCharge;

            if (item.getCellRef().mEnchantmentCharge < castCost)
            {
                // TODO: Should there be a sound here?
                if (mCaster.getRefData().getHandle() == "player")
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInsufficientCharge}");
                return false;
            }

            // Reduce charge
            item.getCellRef().mEnchantmentCharge -= castCost;
        }
        if (enchantment->mData.mType == ESM::Enchantment::CastOnce)
            item.getContainerStore()->remove(item, 1, mCaster);
        else if (enchantment->mData.mType != ESM::Enchantment::WhenStrikes)
        {
            if (mCaster.getRefData().getHandle() == "player")
                MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(item); // Set again to show the modified charge
        }

        if (mCaster.getRefData().getHandle() == "player")
            mCaster.getClass().skillUsageSucceeded (mCaster, ESM::Skill::Enchant, 1);

        inflict(mCaster, mCaster, enchantment->mEffects, ESM::RT_Self);

        if (!mTarget.isEmpty())
        {
            if (!mTarget.getClass().isActor() || !mTarget.getClass().getCreatureStats(mTarget).isDead())
                inflict(mTarget, mCaster, enchantment->mEffects, ESM::RT_Touch);
        }

        MWBase::Environment::get().getWorld()->launchProjectile(mId, false, enchantment->mEffects, mCaster, mSourceName);

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
            fatigue.setCurrent(std::max(0.f, fatigue.getCurrent() - fatigueLoss));
            stats.setFatigue(fatigue);

            bool fail = false;

            // Check success
            int successChance = getSpellSuccessChance(spell, mCaster);
            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            if (!fail && roll >= successChance)
            {
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

        MWBase::Environment::get().getWorld()->launchProjectile(mId, false, spell->mEffects, mCaster, mSourceName);
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
            if (!magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)
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
