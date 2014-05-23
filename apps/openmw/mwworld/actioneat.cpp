
#include "actioneat.hpp"

#include <cstdlib>

#include <components/esm/loadskil.hpp>

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
        std::string id = getTarget().getClass().getId (getTarget());

        if (actor.getClass().apply (actor, id, actor))
            actor.getClass().skillUsageSucceeded (actor, ESM::Skill::Alchemy, 1);
    }

    ActionEat::ActionEat (const MWWorld::Ptr& object) : Action (false, object) {}
}
