#include "recharge.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    bool rechargeItem(const MWWorld::Ptr& item, const float maxCharge, const float duration)
    {
        float charge = item.getCellRef().getEnchantmentCharge();
        if (charge == -1 || charge == maxCharge)
            return false;

        static const float fMagicItemRechargePerSecond = MWBase::Environment::get()
                                                             .getESMStore()
                                                             ->get<ESM::GameSetting>()
                                                             .find("fMagicItemRechargePerSecond")
                                                             ->mValue.getFloat();

        item.getCellRef().setEnchantmentCharge(std::min(charge + fMagicItemRechargePerSecond * duration, maxCharge));
        return true;
    }

    bool rechargeItem(const MWWorld::Ptr& item, const MWWorld::Ptr& gem)
    {
        if (!gem.getCellRef().getCount())
            return false;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);

        MWBase::Environment::get().getWorld()->breakInvisibility(player);

        float luckTerm = 0.1f * stats.getAttribute(ESM::Attribute::Luck).getModified();
        if (luckTerm < 1 || luckTerm > 10)
            luckTerm = 1;

        float intelligenceTerm = 0.2f * stats.getAttribute(ESM::Attribute::Intelligence).getModified();

        if (intelligenceTerm > 20)
            intelligenceTerm = 20;
        if (intelligenceTerm < 1)
            intelligenceTerm = 1;

        float x = (player.getClass().getSkill(player, ESM::Skill::Enchant) + intelligenceTerm + luckTerm)
            * stats.getFatigueTerm();
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        int roll = Misc::Rng::roll0to99(prng);
        if (roll < x)
        {
            const ESM::RefId& soul = gem.getCellRef().getSoul();
            const ESM::Creature* creature = MWBase::Environment::get().getESMStore()->get<ESM::Creature>().find(soul);

            float restored = creature->mData.mSoul * (roll / x);

            const ESM::Enchantment* enchantment
                = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().find(
                    item.getClass().getEnchantment(item));
            const int maxCharge = MWMechanics::getEnchantmentCharge(*enchantment);
            item.getCellRef().setEnchantmentCharge(
                std::min(item.getCellRef().getEnchantmentCharge() + restored, static_cast<float>(maxCharge)));

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Enchant Success"));

            player.getClass().getContainerStore(player).restack(item);
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Enchant Fail"));
        }

        player.getClass().skillUsageSucceeded(player, ESM::Skill::Enchant, ESM::Skill::Enchant_Recharge);
        gem.getContainerStore()->remove(gem, 1);

        if (gem.getCellRef().getCount() == 0)
        {
            std::string message = MWBase::Environment::get()
                                      .getESMStore()
                                      ->get<ESM::GameSetting>()
                                      .find("sNotifyMessage51")
                                      ->mValue.getString();
            message = Misc::StringUtils::format(message, gem.getClass().getName(gem));

            MWBase::Environment::get().getWindowManager()->messageBox(message);

            const ESM::RefId soulGemAzura = ESM::RefId::stringRefId("Misc_SoulGem_Azura");
            // special case: readd Azura's Star
            if (gem.get<ESM::Miscellaneous>()->mBase->mId == soulGemAzura)
                player.getClass().getContainerStore(player).add(soulGemAzura, 1);
        }

        return true;
    }

}
