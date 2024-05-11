#include "classes.hpp"

#include <components/esm4/loadacti.hpp>
#include <components/esm4/loadalch.hpp>
#include <components/esm4/loadammo.hpp>
#include <components/esm4/loadarmo.hpp>
#include <components/esm4/loadbook.hpp>
#include <components/esm4/loadclot.hpp>
#include <components/esm4/loadcont.hpp>
#include <components/esm4/loadcrea.hpp>
#include <components/esm4/loaddoor.hpp>
#include <components/esm4/loadflor.hpp>
#include <components/esm4/loadfurn.hpp>
#include <components/esm4/loadimod.hpp>
#include <components/esm4/loadingr.hpp>
#include <components/esm4/loadligh.hpp>
#include <components/esm4/loadmisc.hpp>
#include <components/esm4/loadmstt.hpp>
#include <components/esm4/loadnpc.hpp>
#include <components/esm4/loadscol.hpp>
#include <components/esm4/loadstat.hpp>
#include <components/esm4/loadterm.hpp>
#include <components/esm4/loadtree.hpp>
#include <components/esm4/loadweap.hpp>

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
#include "lockpick.hpp"
#include "misc.hpp"
#include "npc.hpp"
#include "potion.hpp"
#include "probe.hpp"
#include "repair.hpp"
#include "static.hpp"
#include "weapon.hpp"

#include "esm4base.hpp"
#include "esm4npc.hpp"
#include "light4.hpp"

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

        ESM4Named<ESM4::Activator>::registerSelf();
        ESM4Named<ESM4::Ammunition>::registerSelf();
        ESM4Named<ESM4::Armor>::registerSelf();
        ESM4Named<ESM4::Book>::registerSelf();
        ESM4Named<ESM4::Clothing>::registerSelf();
        ESM4Named<ESM4::Creature>::registerSelf();
        ESM4Named<ESM4::Container>::registerSelf();
        ESM4Named<ESM4::Door>::registerSelf();
        ESM4Named<ESM4::Flora>::registerSelf();
        ESM4Named<ESM4::Furniture>::registerSelf();
        ESM4Named<ESM4::Ingredient>::registerSelf();
        ESM4Named<ESM4::ItemMod>::registerSelf();
        ESM4Light::registerSelf();
        ESM4Named<ESM4::MiscItem>::registerSelf();
        ESM4Named<ESM4::MovableStatic>::registerSelf();
        ESM4Npc::registerSelf();
        ESM4Named<ESM4::Potion>::registerSelf();
        ESM4Static::registerSelf();
        ESM4Named<ESM4::StaticCollection>::registerSelf();
        ESM4Named<ESM4::Terminal>::registerSelf();
        ESM4Tree::registerSelf();
        ESM4Named<ESM4::Weapon>::registerSelf();
    }
}
