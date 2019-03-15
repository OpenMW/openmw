#include "actionalchemy.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{
    ActionAlchemy::ActionAlchemy(bool force)
    : Action (false)
    , mForce(force)
    {
    }

    void ActionAlchemy::executeImp (const Ptr& actor)
    {
        if (actor != MWMechanics::getPlayer())
            return;

        if(!mForce && MWMechanics::isPlayerInCombat())
        { //Ensure we're not in combat
            MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage3}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Alchemy);
    }
}
