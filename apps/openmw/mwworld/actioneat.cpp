
#include "actioneat.hpp"

#include <cstdlib>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/containerstore.hpp"

#include "class.hpp"

namespace MWWorld
{
    void ActionEat::executeImp (const Ptr& actor)
    {
        // remove used item (assume the item is present in inventory)
        getTarget().getContainerStore()->remove(getTarget(), 1, actor);

        // apply to actor
        std::string id = Class::get (getTarget()).getId (getTarget());
            
        if (Class::get (actor).apply (actor, id, actor))
            Class::get (actor).skillUsageSucceeded (actor, ESM::Skill::Alchemy, 1);
    }    

    ActionEat::ActionEat (const MWWorld::Ptr& object) : Action (false, object) {}
}
