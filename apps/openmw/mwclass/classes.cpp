
#include "classes.hpp"

#include "activator.hpp"
#include "creature.hpp"
#include "npc.hpp"
#include "weapon.hpp"
#include "armor.hpp"

namespace MWClass
{
    void registerClasses()
    {
        Activator::registerSelf();
        Creature::registerSelf();
        Npc::registerSelf();
        Weapon::registerSelf();
        Armor::registerSelf();
    }
}
