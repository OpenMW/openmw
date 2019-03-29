#include "actionsoulgem.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{

    ActionSoulgem::ActionSoulgem(const Ptr &object)
        : Action(false, object)
    {

    }

    void ActionSoulgem::executeImp(const Ptr &actor)
    {
        if (actor != MWMechanics::getPlayer())
            return;

        if(MWMechanics::isPlayerInCombat()) { //Ensure we're not in combat
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage5}");
            return;
        }
        MWBase::Environment::get().getWindowManager()->showSoulgemDialog(getTarget());
    }


}
