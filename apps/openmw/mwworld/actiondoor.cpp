#include "actiondoor.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWWorld
{
    ActionDoor::ActionDoor (const MWWorld::Ptr& object) : Action (false, object)
    {
    }

    void ActionDoor::executeImp (const MWWorld::Ptr& actor)
    {
        MWBase::Environment::get().getWorld()->activateDoor(getTarget());
    }
}
