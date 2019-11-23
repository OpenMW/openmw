#ifndef OPENMW_WEAPON_PRIORITY_H
#define OPENMW_WEAPON_PRIORITY_H

#include <components/esm/loadweap.hpp>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    float rateWeapon (const MWWorld::Ptr& item, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy,
                      int type=-1, float arrowRating=0.f, float boltRating=0.f);

    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, MWWorld::Ptr &bestAmmo, int ammoType);
    float rateAmmo(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy, int ammoType);

    float vanillaRateWeaponAndAmmo(const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
}

#endif
