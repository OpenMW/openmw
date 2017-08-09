#ifndef OPENMW_MWMECHANICS_ACTORUTIL_H
#define OPENMW_MWMECHANICS_ACTORUTIL_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    MWWorld::Ptr getPlayer();
    bool isPlayerInCombat();
}

#endif
