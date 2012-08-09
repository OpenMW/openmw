
#include "actiontake.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwgui/window_manager.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionTake::ActionTake (const MWWorld::Ptr& object) : mObject (object) {}

    void ActionTake::executeImp (const Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        // insert into player's inventory
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPtr ("player", true);

        MWWorld::Class::get (player).getContainerStore (player).add (mObject);

        // remove from world, if the item is currently in the world (it could also be in a container)
        if (mObject.isInCell())
            MWBase::Environment::get().getWorld()->deleteObject (mObject);
    }
}
