
#include "actiontake.hpp"

#include "../mwbase/environment.hpp"

#include "class.hpp"
#include "world.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionTake::ActionTake (const MWWorld::Ptr& object) : mObject (object) {}

    void ActionTake::execute()
    {
        // insert into player's inventory
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPtr ("player", true);

        MWWorld::Class::get (player).getContainerStore (player).add (mObject);

        // remove from world, if the item is currently in the world (it could also be in a container)
        if (mObject.isInCell())
            MWBase::Environment::get().getWorld()->deleteObject (mObject);
    }
}
