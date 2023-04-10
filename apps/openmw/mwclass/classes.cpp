#include "classes.hpp"

#include <components/esm/records.hpp>

#include "activator.hpp"
#include "apparatus.hpp"
#include "armor.hpp"
#include "bodypart.hpp"
#include "book.hpp"
#include "clothing.hpp"
#include "container.hpp"
#include "creature.hpp"
#include "creaturelevlist.hpp"
#include "door.hpp"
#include "ingredient.hpp"
#include "itemlevlist.hpp"
#include "light.hpp"
#include "light4.hpp"
#include "lockpick.hpp"
#include "misc.hpp"
#include "npc.hpp"
#include "potion.hpp"
#include "probe.hpp"
#include "repair.hpp"
#include "static.hpp"
#include "weapon.hpp"

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

        ESM4Static<ESM4::Activator>::registerSelf();
        ESM4Static<ESM4::Potion>::registerSelf();
        ESM4Static<ESM4::Ammunition>::registerSelf();
        ESM4Static<ESM4::Armor>::registerSelf();
        ESM4Static<ESM4::Book>::registerSelf();
        ESM4Static<ESM4::Clothing>::registerSelf();
        ESM4Static<ESM4::Container>::registerSelf();
        ESM4Static<ESM4::Door>::registerSelf();
        ESM4Static<ESM4::Ingredient>::registerSelf();
        ESM4Static<ESM4::MiscItem>::registerSelf();
        ESM4Static<ESM4::Static>::registerSelf();
        ESM4Static<ESM4::Weapon>::registerSelf();
        ESM4Light::registerSelf();
    }
}
