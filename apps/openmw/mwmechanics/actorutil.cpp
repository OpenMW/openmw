#include "actorutil.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

namespace MWMechanics
{
    bool isPlayer(const MWWorld::Ptr& ptr)
    {
        return ptr == MWBase::Environment::get().getWorld()->getPlayerPtr();
    }
}
