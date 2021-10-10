#include "spellabsorption.hpp"

#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    class GetAbsorptionProbability : public MWMechanics::EffectSourceVisitor
    {
    public:
        float mProbability{0.f};

        GetAbsorptionProbability() = default;

        void visit (MWMechanics::EffectKey key, int /*effectIndex*/,
                            const std::string& /*sourceName*/, const std::string& /*sourceId*/, int /*casterActorId*/,
                            float magnitude, float /*remainingTime*/, float /*totalTime*/) override
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

    int getAbsorbChance(const MWWorld::Ptr& caster, const MWWorld::Ptr& target)
    {
        if(target.isEmpty() || caster == target || !target.getClass().isActor())
            return 0;

        CreatureStats& stats = target.getClass().getCreatureStats(target);
        if (stats.getMagicEffects().get(ESM::MagicEffect::SpellAbsorption).getMagnitude() <= 0.f)
            return 0;

        GetAbsorptionProbability check;
        stats.getActiveSpells().visitEffectSources(check);
        stats.getSpells().visitEffectSources(check);
        if (target.getClass().hasInventoryStore(target))
            target.getClass().getInventoryStore(target).visitEffectSources(check);

        return check.mProbability * 100;
    }

    void absorbSpell (const std::string& spellId, const MWWorld::Ptr& caster, const MWWorld::Ptr& target)
    {
        CreatureStats& stats = target.getClass().getCreatureStats(target);

        const auto& esmStore = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Static* absorbStatic = esmStore.get<ESM::Static>().find("VFX_Absorb");
        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);
        if (animation && !absorbStatic->mModel.empty())
            animation->addEffect( "meshes\\" + absorbStatic->mModel, ESM::MagicEffect::SpellAbsorption, false, std::string());
        const ESM::Spell* spell = esmStore.get<ESM::Spell>().search(spellId);
        int spellCost = 0;
        if (spell)
        {
            spellCost = spell->mData.mCost;
        }
        else
        {
            const ESM::Enchantment* enchantment = esmStore.get<ESM::Enchantment>().search(spellId);
            if (enchantment)
                spellCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), caster);
        }

        // Magicka is increased by the cost of the spell
        DynamicStat<float> magicka = stats.getMagicka();
        magicka.setCurrent(magicka.getCurrent() + spellCost);
        stats.setMagicka(magicka);
    }

}
