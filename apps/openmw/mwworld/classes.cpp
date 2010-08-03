
#include "classes.hpp"

#include "activator.hpp"
#include "creature.hpp"
#include "npc.hpp"

namespace MWWorld
{
    void registerClasses()
    {
        Activator::registerSelf();
        Creature::registerSelf();
        Npc::registerSelf();
    }
}
