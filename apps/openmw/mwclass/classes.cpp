#include "classes.hpp"

#include "activator.hpp"
#include "creature.hpp"
#include "npc.hpp"
#include "weapon.hpp"
#include "armor.hpp"
#include "potion.hpp"
#include "apparatus.hpp"
#include "book.hpp"
#include "clothing.hpp"
#include "container.hpp"
#include "door.hpp"
#include "ingredient.hpp"
#include "creaturelevlist.hpp"
#include "itemlevlist.hpp"
#include "light.hpp"
#include "lockpick.hpp"
#include "misc.hpp"
#include "probe.hpp"
#include "repair.hpp"
#include "static.hpp"
#include "bodypart.hpp"

namespace MWClass
{
    void registerClasses()
    {
        Activator::registerSelf();
        Creature::registerSelf();
        Npc::registerSelf();
        Weapon::registerSelf();
        Armor::registerSelf();
        Potion::registerSelf();
        Apparatus::registerSelf();
        Book::registerSelf();
        Clothing::registerSelf();
        Container::registerSelf();
        Door::registerSelf();
        Ingredient::registerSelf();
        CreatureLevList::registerSelf();
        ItemLevList::registerSelf();
        Light::registerSelf();
        Lockpick::registerSelf();
        Miscellaneous::registerSelf();
        Probe::registerSelf();
        Repair::registerSelf();
        Static::registerSelf();
        BodyPart::registerSelf();
    }
}
