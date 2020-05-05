#include "spellabsorption.hpp"

#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{

    class GetAbsorptionProbability : public MWMechanics::EffectSourceVisitor
    {
    public:
        float mProbability{0.f};

        GetAbsorptionProbability() = default;

        virtual void visit (MWMechanics::EffectKey key,
                                const std::string& /*sourceName*/, const std::string& /*sourceId*/, int /*casterActorId*/,
                            float magnitude, float /*remainingTime*/, float /*totalTime*/)
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

    bool absorbSpell (const ESM::Spell* spell, const MWWorld::Ptr& caster, const MWWorld::Ptr& target)
    {
        if (!spell || caster == target || !target.getClass().isActor())
            return false;

        CreatureStats& stats = target.getClass().getCreatureStats(target);
        if (stats.getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).getMagnitude() <= 0.f)
            return false;

        GetAbsorptionProbability check;
        stats.getActiveSpells().visitEffectSources(check);
        stats.getSpells().visitEffectSources(check);
        if (target.getClass().hasInventoryStore(target))
            target.getClass().getInventoryStore(target).visitEffectSources(check);

        int chance = check.mProbability * 100;
        if (Misc::Rng::roll0to99() >= chance)
            return false;

        const ESM::Static* absorbStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find("VFX_Absorb");
        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);
        if (animation && !absorbStatic->mModel.empty())
            animation->addEffect( "meshes\\" + absorbStatic->mModel, ESM::MagicEffect::SpellAbsorption, false, std::string());
        // Magicka is increased by the cost of the spell
        DynamicStat<float> magicka = stats.getMagicka();
        magicka.setCurrent(magicka.getCurrent() + spell->mData.mCost);
        stats.setMagicka(magicka);
        return true;
    }

}
