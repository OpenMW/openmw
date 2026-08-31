#ifndef OPENMW_WEAPON_PRIORITY_H
#define OPENMW_WEAPON_PRIORITY_H

#include <components/esm/refid.hpp>

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    float rateWeapon(const MWWorld::Ptr& item, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy,
        ESM::RefId typeId = {}, float arrowRating = 0.f, float boltRating = 0.f);

    float rateAmmo(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, MWWorld::Ptr& bestAmmo, ESM::RefId ammoType);
    float rateAmmo(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, ESM::RefId ammoType);

    float vanillaRateWeaponAndAmmo(
        const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy);
}

#endif
