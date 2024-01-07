#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include <string_view>
#include <vector>

namespace ESM
{
    struct WeaponType;
}

namespace MWWorld
{
    class Ptr;

    template <class PtrType>
    class ContainerStoreIteratorBase;

    using ContainerStoreIterator = ContainerStoreIteratorBase<Ptr>;
}

namespace MWMechanics
{
    MWWorld::ContainerStoreIterator getActiveWeapon(const MWWorld::Ptr& actor, int* weaptype);

    const ESM::WeaponType* getWeaponType(const int weaponType);

    std::vector<std::string_view> getAllWeaponTypeShortGroups();
}

#endif
