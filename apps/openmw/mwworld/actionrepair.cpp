#include "actionrepair.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{
    ActionRepair::ActionRepair(const Ptr &item)
        : Action(false, item)
    {
    }

    void ActionRepair::executeImp (const Ptr& actor)
    {
        if (actor != MWMechanics::getPlayer())
            return;

        if(MWMechanics::isPlayerInCombat()) {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage2}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Repair, getTarget());
    }
}
