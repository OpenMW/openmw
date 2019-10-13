#include "recharge.hpp"

#include <components/misc/rng.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "creaturestats.hpp"
#include "actorutil.hpp"

namespace MWMechanics
{

bool rechargeItem(const MWWorld::Ptr &item, const float maxCharge, const float duration)
{
    float charge = item.getCellRef().getEnchantmentCharge();
    if (charge == -1 || charge == maxCharge)
        return false;

    static const float fMagicItemRechargePerSecond = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                "fMagicItemRechargePerSecond")->mValue.getFloat();

    item.getCellRef().setEnchantmentCharge(std::min(charge + fMagicItemRechargePerSecond * duration, maxCharge));
    return true;
}

bool rechargeItem(const MWWorld::Ptr &item, const MWWorld::Ptr &gem)
{
    if (!gem.getRefData().getCount())
        return false;

    MWWorld::Ptr player = MWMechanics::getPlayer();
    MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);

    float luckTerm = 0.1f * stats.getAttribute(ESM::Attribute::Luck).getModified();
    if (luckTerm < 1 || luckTerm > 10)
        luckTerm = 1;

    float intelligenceTerm = 0.2f * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

    if (intelligenceTerm > 20)
        intelligenceTerm = 20;
    if (intelligenceTerm < 1)
        intelligenceTerm = 1;

    float x = (player.getClass().getSkill(player, ESM::Skill::Enchant) + intelligenceTerm + luckTerm) * stats.getFatigueTerm();
    int roll = Misc::Rng::roll0to99();
    if (roll < x)
    {
        std::string soul = gem.getCellRef().getSoul();
        const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(soul);

        float restored = creature->mData.mSoul * (roll / x);

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    item.getClass().getEnchantment(item));
        item.getCellRef().setEnchantmentCharge(
            std::min(item.getCellRef().getEnchantmentCharge() + restored, static_cast<float>(enchantment->mData.mCharge)));

        MWBase::Environment::get().getWindowManager()->playSound("Enchant Success");

        player.getClass().getContainerStore(player).restack(item);
    }
    else
    {
        MWBase::Environment::get().getWindowManager()->playSound("Enchant Fail");
    }

    player.getClass().skillUsageSucceeded (player, ESM::Skill::Enchant, 0);
    gem.getContainerStore()->remove(gem, 1, player);

    if (gem.getRefData().getCount() == 0)
    {
        std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sNotifyMessage51")->mValue.getString();
        message = Misc::StringUtils::format(message, gem.getClass().getName(gem));

        MWBase::Environment::get().getWindowManager()->messageBox(message);

        // special case: readd Azura's Star
        if (Misc::StringUtils::ciEqual(gem.get<ESM::Miscellaneous>()->mBase->mId, "Misc_SoulGem_Azura"))
            player.getClass().getContainerStore(player).add("Misc_SoulGem_Azura", 1, player);
    }

    return true;
}

}
