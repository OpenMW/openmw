
#include "actiontake.hpp"

#include "class.hpp"
#include "environment.hpp"
#include "world.hpp"

namespace MWWorld
{
    ActionTake::ActionTake (const MWWorld::Ptr& object) : mObject (object) {}

    void ActionTake::execute (Environment& environment)
    {
        // insert into player's inventory
        MWWorld::Ptr player = environment.mWorld->getPtr ("player", true);

        MWWorld::Class::get (mObject).insertIntoContainer (mObject,
            MWWorld::Class::get (player).getContainerStore (player));

        // remove from world
        environment.mWorld->deleteObject (mObject);
    }
}
