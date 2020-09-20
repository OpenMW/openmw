#include "linkedeffects.hpp"

#include <components/misc/rng.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{

    bool reflectEffect(const ESM::ENAMstruct& effect, const ESM::MagicEffect* magicEffect,
                       const MWWorld::Ptr& caster, const MWWorld::Ptr& target, ESM::EffectList& reflectedEffects)
    {
        if (caster.isEmpty() || caster == target || !target.getClass().isActor())
            return false;

        bool isHarmful = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful;
        bool isUnreflectable = magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable;
        if (!isHarmful || isUnreflectable)
            return false;

        float reflect = target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Reflect).getMagnitude();
        if (Misc::Rng::roll0to99() >= reflect)
            return false;

        const ESM::Static* reflectStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Reflect");
        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);
        if (animation && !reflectStatic->mModel.empty())
            animation->addEffect("meshes\\" + reflectStatic->mModel, ESM::MagicEffect::Reflect, false, std::string());
        reflectedEffects.mList.emplace_back(effect);
        return true;
    }

    void absorbStat(const ESM::ENAMstruct& effect, const ESM::ActiveEffect& appliedEffect,
                    const MWWorld::Ptr& caster, const MWWorld::Ptr& target, bool reflected, const std::string& source)
    {
        if (caster.isEmpty() || caster == target)
            return;

        if (!target.getClass().isActor() || !caster.getClass().isActor())
            return;

        // Make sure callers don't do something weird
        if (effect.mEffectID < ESM::MagicEffect::AbsorbAttribute || effect.mEffectID > ESM::MagicEffect::AbsorbSkill)
            throw std::runtime_error("invalid absorb stat effect");

        if (appliedEffect.mMagnitude == 0)
            return;

        std::vector<ActiveSpells::ActiveEffect> absorbEffects;
        ActiveSpells::ActiveEffect absorbEffect = appliedEffect;
        absorbEffect.mMagnitude *= -1;
        absorbEffect.mEffectIndex = appliedEffect.mEffectIndex;
        absorbEffects.emplace_back(absorbEffect);

        // Morrowind negates reflected Absorb spells so the original caster won't be harmed.
        if (reflected && Settings::Manager::getBool("classic reflected absorb spells behavior", "Game"))
        {
            target.getClass().getCreatureStats(target).getActiveSpells().addSpell(std::string(), true,
                            absorbEffects, source, caster.getClass().getCreatureStats(caster).getActorId());
            return;
        }

        caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell(std::string(), true,
                        absorbEffects, source, target.getClass().getCreatureStats(target).getActorId());
    }
}
