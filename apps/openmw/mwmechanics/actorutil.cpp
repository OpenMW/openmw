#include "actorutil.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWMechanics
{
    MWWorld::Ptr getPlayer()
    {
        return MWBase::Environment::get().getWorld()->getPlayerPtr();
    }
}
