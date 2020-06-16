#include "spellresistance.hpp"

#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "creaturestats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    float getEffectMultiplier(short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                              const ESM::Spell* spell, const MagicEffects* effects)
    {
        if (!actor.getClass().isActor())
            return 1;

        float resistance = getEffectResistance(effectId, actor, caster, spell, effects);
        return 1 - resistance / 100.f;
    }

    float getEffectResistance (short effectId, const MWWorld::Ptr& actor, const MWWorld::Ptr& caster,
                               const ESM::Spell* spell, const MagicEffects* effects)
    {
        // Effects with no resistance attribute belonging to them can not be resisted
        if (ESM::MagicEffect::getResistanceEffect(effectId) == -1)
            return 0.f;

        const auto magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectId);

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

}
