#include "actionrepair.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

namespace MWWorld
{
    ActionRepair::ActionRepair(const Ptr &item)
        : Action(false, item)
    {
    }

    void ActionRepair::executeImp (const Ptr& actor)
    {
        if(MWBase::Environment::get().getWorld()->getPlayer().isInCombat()) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage2}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->startRepairItem(getTarget());
    }
}
