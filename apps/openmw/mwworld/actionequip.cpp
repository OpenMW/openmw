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
        if (!result.second.empty() && actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
            MWBase::Environment::get().getWindowManager()->messageBox(result.second);

        switch(result.first)
        {
            case 0:
                return;
            case 2:
                invStore.unequipSlot(MWWorld::InventoryStore::Slot_CarriedLeft, actor);
                break;
            case 3:
                invStore.unequipSlot(MWWorld::InventoryStore::Slot_CarriedRight, actor);
                break;
        }

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = MWWorld::Class::get(getTarget()).getEquipmentSlots(getTarget());

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
        for (std::vector<int>::const_iterator slot=slots_.first.begin();
            slot!=slots_.first.end(); ++slot)
        {
            // if the item is equipped already, nothing to do
            if (invStore.getSlot(*slot) == it)
                return;

            // if all slots are occupied, replace the last slot
            if (slot == --slots_.first.end())
            {
                invStore.equip(*slot, it, actor);
                equipped = true;
                break;
            }

            if (invStore.getSlot(*slot) == invStore.end())
            {
                // slot is not occupied
                invStore.equip(*slot, it, actor);
                equipped = true;
                break;
            }
        }

        std::string script = MWWorld::Class::get(object).getScript(object);
        
        /* Set OnPCEquip Variable on item's script, if the player is equipping it, and it has a script with that variable declared */
        if(equipped && actor == MWBase::Environment::get().getWorld()->getPlayerPtr() && script != "")
            object.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);
    }
}
