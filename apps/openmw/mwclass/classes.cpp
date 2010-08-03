
#include "classes.hpp"

#include "activator.hpp"
#include "creature.hpp"
#include "npc.hpp"

namespace MWClass
{
    void registerClasses()
    {
        Activator::registerSelf();
        Creature::registerSelf();
        Npc::registerSelf();
    }
}
