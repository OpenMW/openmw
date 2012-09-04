
#include "actiontake.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionTake::ActionTake (const MWWorld::Ptr& object) : Action (false, object) {}

    void ActionTake::executeImp (const Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        // insert into player's inventory
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPtr ("player", true);

        MWWorld::Class::get (player).getContainerStore (player).add (getTarget());

        MWBase::Environment::get().getWorld()->deleteObject (getTarget());
    }
}
