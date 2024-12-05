#include "repair.hpp"

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

namespace MWMechanics
{

    void Repair::repair(const MWWorld::Ptr& itemToRepair)
    {
        MWWorld::Ptr player = getPlayer();
        MWWorld::LiveCellRef<ESM::Repair>* ref = mTool.get<ESM::Repair>();

        MWBase::Environment::get().getWorld()->breakInvisibility(player);

        // unstack tool if required
        player.getClass().getContainerStore(player).unstack(mTool);

        // reduce number of uses left
        int uses = mTool.getClass().getItemHealth(mTool);
        uses -= std::min(uses, 1);
        mTool.getCellRef().setCharge(uses);

        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);

        float fatigueTerm = stats.getFatigueTerm();
        float pcStrength = stats.getAttribute(ESM::Attribute::Strength).getModified();
        float pcLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();
        float armorerSkill = player.getClass().getSkill(player, ESM::Skill::Armorer);

        float fRepairAmountMult = MWBase::Environment::get()
                                      .getESMStore()
                                      ->get<ESM::GameSetting>()
                                      .find("fRepairAmountMult")
                                      ->mValue.getFloat();

        float toolQuality = ref->mBase->mData.mQuality;

        float x = (0.1f * pcStrength + 0.1f * pcLuck + armorerSkill) * fatigueTerm;

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        int roll = Misc::Rng::roll0to99(prng);
        if (roll <= x)
        {
            int y = static_cast<int>(fRepairAmountMult * toolQuality * roll);
            y = std::max(1, y);

            // repair by 'y' points
            int charge = itemToRepair.getClass().getItemHealth(itemToRepair);
            charge = std::min(charge + y, itemToRepair.getClass().getItemMaxHealth(itemToRepair));
            itemToRepair.getCellRef().setCharge(charge);

            // attempt to re-stack item, in case it was fully repaired
            MWWorld::ContainerStoreIterator stacked = player.getClass().getContainerStore(player).restack(itemToRepair);

            // set the OnPCRepair variable on the item's script
            const ESM::RefId& script = stacked->getClass().getScript(itemToRepair);
            if (!script.empty())
                stacked->getRefData().getLocals().setVarByInt(script, "onpcrepair", 1);

            // increase skill
            player.getClass().skillUsageSucceeded(player, ESM::Skill::Armorer, ESM::Skill::Armorer_Repair);

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Repair"));
            MWBase::Environment::get().getWindowManager()->messageBox("#{sRepairSuccess}");
        }
        else
        {
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Repair Fail"));
            MWBase::Environment::get().getWindowManager()->messageBox("#{sRepairFailed}");
        }

        // tool used up?
        if (mTool.getCellRef().getCharge() == 0)
        {
            MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);

            store.remove(mTool, 1);

            std::string message = MWBase::Environment::get()
                                      .getESMStore()
                                      ->get<ESM::GameSetting>()
                                      .find("sNotifyMessage51")
                                      ->mValue.getString();
            message = Misc::StringUtils::format(message, mTool.getClass().getName(mTool));

            MWBase::Environment::get().getWindowManager()->messageBox(message);

            // try to find a new tool of the same ID
            for (MWWorld::ContainerStoreIterator iter(store.begin()); iter != store.end(); ++iter)
            {
                if (iter->getCellRef().getRefId() == mTool.getCellRef().getRefId())
                {
                    mTool = *iter;

                    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Item Repair Up"));

                    break;
                }
            }
        }
    }

}
