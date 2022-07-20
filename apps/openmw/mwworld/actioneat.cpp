#include "actioneat.hpp"

#include <components/esm3/loadskil.hpp>

#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "class.hpp"

namespace MWWorld
{
    void ActionEat::executeImp (const Ptr& actor)
    {
        if (actor.getClass().consume(getTarget(), actor) && actor == MWMechanics::getPlayer())
            actor.getClass().skillUsageSucceeded (actor, ESM::Skill::Alchemy, 1);
    }

    ActionEat::ActionEat (const MWWorld::Ptr& object) : Action (false, object) {}
}
