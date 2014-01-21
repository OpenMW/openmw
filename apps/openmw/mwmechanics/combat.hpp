#ifndef OPENMW_MECHANICS_COMBAT_H
#define OPENMW_MECHANICS_COMBAT_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

/// @return can we block the attack?
bool blockMeleeAttack (const MWWorld::Ptr& attacker, const MWWorld::Ptr& blocker, const MWWorld::Ptr& weapon, float damage);

}

#endif
