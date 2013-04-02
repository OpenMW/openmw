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

        bool equipped = false;

        // equip the item in the first free slot
        for (std::vector<int>::const_iterator slot=slots.first.begin();
            slot!=slots.first.end(); ++slot)
        {

            // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
            const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npcRace);
            if(race->mData.mFlags & ESM::Race::Beast)
            {
                if(*slot == MWWorld::InventoryStore::Slot_Helmet)
                {
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
                                MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage13}");

                            allow = false;
                            break;
                        }
                    }

                    if(!allow)
                        break;
                }

                if (*slot == MWWorld::InventoryStore::Slot_Boots)
                {
                    bool allow = true;
                    std::vector<ESM::PartReference> parts;
                    if(it.getType() == MWWorld::ContainerStore::Type_Clothing)
                        parts = it->get<ESM::Clothing>()->mBase->mParts.mParts;
                    else
                        parts = it->get<ESM::Armor>()->mBase->mParts.mParts;
                    for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
                    {
                        if((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
                        {
                            allow = false;
                            // Only notify the player, not npcs
                            if(actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer() )
                            {
                                if(it.getType() == MWWorld::ContainerStore::Type_Clothing){ // It's shoes
                                    MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage15}");
                                }

                                else // It's boots
                                {
                                    MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage14}");
                                }
                            }
                            break;
                        }
                    }

                    if(!allow)
                        break;
                }

            }

            //Disable twohanded when shield equipped, shield when twohanded equipped
            if(*slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                if (it.getType() == MWWorld::ContainerStore::Type_Weapon)
                {
                    if(it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose || 
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide || 
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand || 
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow || 
                    it->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
                    {
                        //unequip lefthand item
                        invStore.equip(MWWorld::InventoryStore::Slot_CarriedLeft, invStore.end());
                    }
                }
            }
            if(*slot == MWWorld::InventoryStore::Slot_CarriedLeft)
            {
                ContainerStoreIterator weapon = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

                if (weapon.getType() == MWWorld::ContainerStore::Type_Weapon)
                {
                    if(weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose || 
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide || 
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand || 
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow || 
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
                    {
                        //unequip twohanded item
                        invStore.equip(MWWorld::InventoryStore::Slot_CarriedRight, invStore.end());
                    }
                }
            }

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

        std::string script = MWWorld::Class::get(*it).getScript(*it);
        
        /* Set OnPCEquip Variable on item's script, if the player is equipping it, and it has a script with that variable declared */
        if(equipped && actor == MWBase::Environment::get().getWorld()->getPlayer().getPlayer() && script != "")
            (*it).mRefData->getLocals().setVarByInt(script, "onpcequip", 1);
    }
}
