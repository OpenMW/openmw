#ifndef OPENMW_MWMECHANICS_DIFFICULTYSCALING_H
#define OPENMW_MWMECHANICS_DIFFICULTYSCALING_H

namespace MWWorld
{
    class Ptr;
}

/// Scales damage dealt to an actor based on difficulty setting
float scaleDamage(float damage, const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim);

#endif
