#include "spellresistance.hpp"

#include <components/misc/rng.hpp>

#include <components/esm3/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "creaturestats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    float getEffectMultiplier(const ESM::RefId& effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
        const ESM::Spell* spell, const MagicEffects* effects)
    {
        if (!actor.getClass().isActor())
            return 1;

        float resistance = getEffectResistance(effectId, actor, caster, spell, effects);
        return 1 - resistance / 100.f;
    }

    float getEffectResistance(const ESM::RefId& effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
        const ESM::Spell* spell, const MagicEffects* effects)
    {
        // Effects with no resistance attribute belonging to them can not be resisted
        if (effectId.empty() || ESM::MagicEffect::getResistanceEffect(*effectId.getIf<ESM::MagicEffectId>()).empty())
            return 0.f;

        const auto magicEffect = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>().search(effectId);

        const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWMechanics::MagicEffects* magicEffects = &stats.getMagicEffects();
        if (effects)
            magicEffects = effects;

        float resistance = getEffectResistanceAttribute(effectId, magicEffects);

        float willpower = stats.getAttribute(ESM::Attribute::Willpower).getModified();
        float luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
        float x = (willpower + 0.1f * luck) * stats.getFatigueTerm();

        // This makes spells that are easy to cast harder to resist and vice versa
        float castChance = 100.f;
        if (spell != nullptr && !caster.isEmpty() && caster.getClass().isActor())
            castChance = getSpellSuccessChance(spell, caster, nullptr, false, false); // Uncapped casting chance
        if (castChance > 0)
            x *= 50 / castChance;

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        float roll = Misc::Rng::rollClosedProbability(prng) * 100;
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

    float getEffectResistanceAttribute(const ESM::RefId& effectId, const MagicEffects* actorEffects)
    {
        float resistance = 0;
        if (effectId.empty())
            return resistance;

        ESM::RefId resistanceEffect = ESM::MagicEffect::getResistanceEffect(*effectId.getIf<ESM::MagicEffectId>());
        ESM::RefId weaknessEffect = ESM::MagicEffect::getWeaknessEffect(*effectId.getIf<ESM::MagicEffectId>());

        if (!resistanceEffect.empty())
            resistance += actorEffects->getOrDefault(*resistanceEffect.getIf<ESM::MagicEffectId>()).getMagnitude();
        if (!weaknessEffect.empty())
            resistance -= actorEffects->getOrDefault(*weaknessEffect.getIf<ESM::MagicEffectId>()).getMagnitude();

        if (effectId == ESM::MagicEffect::FireDamage)
            resistance += actorEffects->getOrDefault(ESM::MagicEffect::FireShield).getMagnitude();
        if (effectId == ESM::MagicEffect::ShockDamage)
            resistance += actorEffects->getOrDefault(ESM::MagicEffect::LightningShield).getMagnitude();
        if (effectId == ESM::MagicEffect::FrostDamage)
            resistance += actorEffects->getOrDefault(ESM::MagicEffect::FrostShield).getMagnitude();

        return resistance;
    }

}
