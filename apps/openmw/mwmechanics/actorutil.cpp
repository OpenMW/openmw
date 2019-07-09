#include "actorutil.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

namespace MWMechanics
{
    MWWorld::Ptr getPlayer()
    {
        return MWBase::Environment::get().getWorld()->getPlayerPtr();
    }

    bool isPlayerInCombat()
    {
        return MWBase::Environment::get().getWorld()->getPlayer().isInCombat();
    }

    bool canActorMoveByZAxis(const MWWorld::Ptr& actor)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        return (actor.getClass().canSwim(actor) && world->isSwimming(actor)) || world->isFlying(actor);
    }
}
