#include "pickpocket.hpp"

#include <components/misc/rng.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "npcstats.hpp"

namespace MWMechanics
{

    Pickpocket::Pickpocket(const MWWorld::Ptr &thief, const MWWorld::Ptr &victim)
        : mThief(thief)
        , mVictim(victim)
    {
    }

    float Pickpocket::getChanceModifier(const MWWorld::Ptr &ptr, float add)
    {
        NpcStats& stats = ptr.getClass().getNpcStats(ptr);
        float agility = stats.getAttribute(ESM::Attribute::Agility).getModified();
        float luck = stats.getAttribute(ESM::Attribute::Luck).getModified();
        float sneak = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Sneak));
        return (add + 0.2f * agility + 0.1f * luck + sneak) * stats.getFatigueTerm();
    }

    bool Pickpocket::getDetected(float valueTerm)
    {
        float x = getChanceModifier(mThief);
        float y = getChanceModifier(mVictim, valueTerm);

        float t = 2*x - y;

        float pcSneak = static_cast<float>(mThief.getClass().getSkill(mThief, ESM::Skill::Sneak));
        int iPickMinChance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("iPickMinChance")->mValue.getInteger();
        int iPickMaxChance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("iPickMaxChance")->mValue.getInteger();

        int roll = Misc::Rng::roll0to99();
        if (t < pcSneak / iPickMinChance)
        {
            return (roll > int(pcSneak / iPickMinChance));
        }
        else
        {
            t = std::min(float(iPickMaxChance), t);
            return (roll > int(t));
        }
    }

    bool Pickpocket::pick(const MWWorld::Ptr& item, int count)
    {
        float stackValue = static_cast<float>(item.getClass().getValue(item) * count);
        float fPickPocketMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fPickPocketMod")->mValue.getFloat();
        float valueTerm = 10 * fPickPocketMod * stackValue;

        return getDetected(valueTerm);
    }

    bool Pickpocket::finish()
    {
        return getDetected(0.f);
    }

}
