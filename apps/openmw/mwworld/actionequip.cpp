#include "actionequip.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

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
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(actor).getInventoryStore(actor);

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots = MWWorld::Class::get(getTarget()).getEquipmentSlots(getTarget());

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = invStore.begin();
        for (; it != invStore.end(); ++it)
        {
            if (*it == getTarget())
            {
                break;
            }
        }

        assert(it != invStore.end());

        std::string npcRace = actor.get<ESM::NPC>()->mBase->mRace;

        // equip the item in the first free slot
        for (std::vector<int>::const_iterator slot=slots.first.begin();
            slot!=slots.first.end(); ++slot)
        {

            // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
            if(npcRace == "argonian" || npcRace == "khajiit")
            {
                if(*slot == MWWorld::InventoryStore::Slot_Helmet){
                    std::vector<ESM::PartReference> parts;

                    if(it.getType() == MWWorld::ContainerStore::Type_Clothing)
                        parts = it->get<ESM::Clothing>()->mBase->mParts.mParts;
                    else
                        parts = it->get<ESM::Armor>()->mBase->mParts.mParts;

                    bool allow = true;
                    for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
                    {
                        if((*itr).mPart == ESM::PRT_Head)
                        {
                            if(actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer() )
                                MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage13}", std::vector<std::string>());

                            allow = false;
                            break;
                        }
                    }

                    if(!allow)
                        break;
                }

                if (*slot == MWWorld::InventoryStore::Slot_Boots)
                {
                    // Only notify the player, not npcs
                    if(actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer() )
                    {
                        if(it.getType() == MWWorld::ContainerStore::Type_Clothing){ // It's shoes
                            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage15}", std::vector<std::string>());
                        }

                        else // It's boots
                        {
                            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage14}", std::vector<std::string>());
                        }
                    }
                    break;
                }

            }

            // if all slots are occupied, replace the last slot
            if (slot == --slots.first.end())
            {
                invStore.equip(*slot, it);
                break;
            }

            if (invStore.getSlot(*slot) == invStore.end())
            {
                // slot is not occupied
                invStore.equip(*slot, it);
                break;
            }
        }
    }
}
