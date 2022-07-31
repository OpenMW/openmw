#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include "../mwworld/inventorystore.hpp"

namespace MWMechanics
{
    MWWorld::ContainerStoreIterator getActiveWeapon(const MWWorld::Ptr& actor, int *weaptype);

    const ESM::WeaponType* getWeaponType(const int weaponType);
}

#endif
