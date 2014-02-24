#include "repair.hpp"

#include <boost/format.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace MWMechanics
{

void Repair::repair(const MWWorld::Ptr &itemToRepair)
{
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
    MWWorld::LiveCellRef<ESM::Repair> *ref =
        mTool.get<ESM::Repair>();

    // unstack tool if required
    player.getClass().getContainerStore(player).unstack(mTool, player);

    // reduce number of uses left
    int uses = (mTool.getCellRef().mCharge != -1) ? mTool.getCellRef().mCharge : ref->mBase->mData.mUses;
    mTool.getCellRef().mCharge = uses-1;

    MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
    MWMechanics::NpcStats& npcStats = MWWorld::Class::get(player).getNpcStats(player);

    float fatigueTerm = stats.getFatigueTerm();
    int pcStrength = stats.getAttribute(ESM::Attribute::Strength).getModified();
    int pcLuck = stats.getAttribute(ESM::Attribute::Luck).getModified();
    int armorerSkill = npcStats.getSkill(ESM::Skill::Armorer).getModified();

    float fRepairAmountMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
            .find("fRepairAmountMult")->getFloat();

    float toolQuality = ref->mBase->mData.mQuality;

    float x = (0.1 * pcStrength + 0.1 * pcLuck + armorerSkill) * fatigueTerm;

    int roll = static_cast<float> (std::rand()) / RAND_MAX * 100;
    if (roll <= x)
    {
        int y = fRepairAmountMult * toolQuality * roll;
        y = std::max(1, y);

        // repair by 'y' points
        itemToRepair.getCellRef().mCharge += y;
        itemToRepair.getCellRef().mCharge = std::min(itemToRepair.getCellRef().mCharge,
                                                     MWWorld::Class::get(itemToRepair).getItemMaxHealth(itemToRepair));

        // set the OnPCRepair variable on the item's script
        std::string script = MWWorld::Class::get(itemToRepair).getScript(itemToRepair);
        if(script != "")
            itemToRepair.getRefData().getLocals().setVarByInt(script, "onpcrepair", 1);

        // increase skill
        MWWorld::Class::get(player).skillUsageSucceeded(player, ESM::Skill::Armorer, 0);

        MWBase::Environment::get().getSoundManager()->playSound("Repair",1,1);
        MWBase::Environment::get().getWindowManager()->messageBox("#{sRepairSuccess}");
    }
    else
    {
        MWBase::Environment::get().getSoundManager()->playSound("Repair Fail",1,1);
        MWBase::Environment::get().getWindowManager()->messageBox("#{sRepairFailed}");
    }

    // tool used up?
    if (mTool.getCellRef().mCharge == 0)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::ContainerStore& store = MWWorld::Class::get(player).getContainerStore(player);

        store.remove(mTool, 1, player);

        std::string message = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("sNotifyMessage51")->getString();

        MWBase::Environment::get().getWindowManager()->messageBox((boost::format(message) % MWWorld::Class::get(mTool).getName(mTool)).str());

        // try to find a new tool of the same ID
        for (MWWorld::ContainerStoreIterator iter (store.begin());
             iter!=store.end(); ++iter)
        {
            if (Misc::StringUtils::ciEqual(iter->getCellRef().mRefID, mTool.getCellRef().mRefID))
            {
                mTool = *iter;
                break;
            }
        }
    }
}

}
