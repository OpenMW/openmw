#include "actionequip.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/compiler/locals.hpp>

#include "inventorystore.hpp"
#include "player.hpp"
#include "class.hpp"

namespace MWWorld
{
    ActionEquip::ActionEquip (const MWWorld::Ptr& object) : Action (false, object)
    {
    }

    void ActionEquip::executeImp (const Ptr& actor)
    {
        MWWorld::Ptr object = getTarget();
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(actor).getInventoryStore(actor);

        std::pair <int, std::string> result = MWWorld::Class::get (object).canBeEquipped (object, actor);

        // display error message if the player tried to equip something
        if (!result.second.empty() && actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer())
            MWBase::Environment::get().getWindowManager()->messageBox(result.second);

        switch(result.first)
        {
            case 0:
                return;
            case 2:
                invStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, invStore.end());
                break;
            case 3:
                invStore.equip(MWWorld::InventoryStore::Slot_CarriedRight, invStore.end());
                break;
        }

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots = MWWorld::Class::get(getTarget()).getEquipmentSlots(getTarget());

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = invStore.begin();
        for (; it != invStore.end(); ++it)
        {
            if (*it == object)
            {
                break;
            }
        }

        assert(it != invStore.end());

        bool equipped = false;

        // equip the item in the first free slot
        for (std::vector<int>::const_iterator slot=slots.first.begin();
            slot!=slots.first.end(); ++slot)
        {

            // if all slots are occupied, replace the last slot
            if (slot == --slots.first.end())
            {
                invStore.equip(*slot, it);
                equipped = true;
                break;
            }

            if (invStore.getSlot(*slot) == invStore.end())
            {
                // slot is not occupied
                invStore.equip(*slot, it);
                equipped = true;
                break;
            }
        }

        std::string script = MWWorld::Class::get(object).getScript(object);
        
        /* Set OnPCEquip Variable on item's script, if the player is equipping it, and it has a script with that variable declared */
        if(equipped && actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer() && script != "")
            object.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);
    }
}
