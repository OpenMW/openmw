
#include "inventorystore.hpp"

#include <iterator>
#include <algorithm>

#include <components/esm/loadench.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "esmstore.hpp"
#include "class.hpp"

void MWWorld::InventoryStore::copySlots (const InventoryStore& store)
{
    // some const-trickery, required because of a flaw in the handling of MW-references and the
    // resulting workarounds
    for (std::vector<ContainerStoreIterator>::const_iterator iter (
        const_cast<InventoryStore&> (store).mSlots.begin());
        iter!=const_cast<InventoryStore&> (store).mSlots.end(); ++iter)
    {
        std::size_t distance = std::distance (const_cast<InventoryStore&> (store).begin(), *iter);

        ContainerStoreIterator slot = begin();

        std::advance (slot, distance);

        mSlots.push_back (slot);
    }
}

void MWWorld::InventoryStore::initSlots (TSlots& slots)
{
    for (int i=0; i<Slots; ++i)
        slots.push_back (end());
}

MWWorld::InventoryStore::InventoryStore() : mMagicEffectsUpToDate (false)
 , mSelectedEnchantItem(end())
 , mActorModelUpdateEnabled (true)
{
    initSlots (mSlots);
}

MWWorld::InventoryStore::InventoryStore (const InventoryStore& store)
: ContainerStore (store)
 , mSelectedEnchantItem(end())
{
    mMagicEffects = store.mMagicEffects;
    mMagicEffectsUpToDate = store.mMagicEffectsUpToDate;
    mSelectedEnchantItem = store.mSelectedEnchantItem;
    copySlots (store);
}

MWWorld::InventoryStore& MWWorld::InventoryStore::operator= (const InventoryStore& store)
{
    mMagicEffects = store.mMagicEffects;
    mMagicEffectsUpToDate = store.mMagicEffectsUpToDate;
    ContainerStore::operator= (store);
    mSlots.clear();
    copySlots (store);
    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::add(const Ptr& itemPtr, const Ptr& actorPtr)
{
    const MWWorld::ContainerStoreIterator& retVal = MWWorld::ContainerStore::add(itemPtr, actorPtr);

    // Auto-equip items if an armor/clothing item is added, but not for the player nor werewolves
    if ((actorPtr.getRefData().getHandle() != "player")
            && !(MWWorld::Class::get(actorPtr).getNpcStats(actorPtr).isWerewolf()))
    {
        std::string type = itemPtr.getTypeName();
        if ((type == typeid(ESM::Armor).name()) || (type == typeid(ESM::Clothing).name()))
            autoEquip(actorPtr);
    }

    return retVal;
}

void MWWorld::InventoryStore::equip (int slot, const ContainerStoreIterator& iterator, const Ptr& actor)
{
    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    if (iterator.getContainerStore()!=this)
        throw std::runtime_error ("attempt to equip an item that is not in the inventory");

    std::pair<std::vector<int>, bool> slots;
    if (iterator!=end())
    {
        slots = Class::get (*iterator).getEquipmentSlots (*iterator);

        if (std::find (slots.first.begin(), slots.first.end(), slot)==slots.first.end())
            throw std::runtime_error ("invalid slot");
    }

    if (mSlots[slot] != end())
        unequipSlot(slot, actor);

    // unstack item pointed to by iterator if required
    if (iterator!=end() && !slots.second && iterator->getRefData().getCount() > 1) // if slots.second is true, item can stay stacked when equipped
    {
        // add the item again with a count of count-1, then set the count of the original (that will be equipped) to 1
        int count = iterator->getRefData().getCount();
        iterator->getRefData().setCount(count-1);
        addNewStack(*iterator);
        iterator->getRefData().setCount(1);
    }

    mSlots[slot] = iterator;

    flagAsModified();

    updateActorModel(actor);
}

void MWWorld::InventoryStore::unequipAll(const MWWorld::Ptr& actor)
{
    for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        unequipSlot(slot, actor);
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getSlot (int slot)
{
    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    if (mSlots[slot]==end())
        return end();

    if (mSlots[slot]->getRefData().getCount()<1)
    {
        // object has been deleted
        mSlots[slot] = end();
        return end();
    }

    return mSlots[slot];
}

void MWWorld::InventoryStore::autoEquip (const MWWorld::Ptr& npc)
{
    const MWMechanics::NpcStats& stats = MWWorld::Class::get(npc).getNpcStats(npc);
    MWWorld::InventoryStore& invStore = MWWorld::Class::get(npc).getInventoryStore(npc);

    TSlots slots;
    initSlots (slots);

    // Disable model update during auto-equip
    mActorModelUpdateEnabled = false;

    for (ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
    {
        Ptr test = *iter;
        int testSkill = MWWorld::Class::get (test).getEquipmentSkill (test);

        std::pair<std::vector<int>, bool> itemsSlots =
            MWWorld::Class::get (*iter).getEquipmentSlots (*iter);

        for (std::vector<int>::const_iterator iter2 (itemsSlots.first.begin());
            iter2!=itemsSlots.first.end(); ++iter2)
        {
            bool use = false;

            if (slots.at (*iter2)==end())
                use = true; // slot was empty before -> skip all further checks
            else
            {
                Ptr old = *slots.at (*iter2);

                if (!use)
                {
                    // check skill
                    int oldSkill =
                        MWWorld::Class::get (old).getEquipmentSkill (old);

                    if (testSkill!=-1 && oldSkill==-1)
                        use = true;
                    else if (testSkill!=-1 && oldSkill!=-1 && testSkill!=oldSkill)
                    {
                        if (stats.getSkill (oldSkill).getModified()>stats.getSkill (testSkill).getModified())
                            continue; // rejected, because old item better matched the NPC's skills.

                        if (stats.getSkill (oldSkill).getModified()<stats.getSkill (testSkill).getModified())
                            use = true;
                    }
                }

                if (!use)
                {
                    // check value
                    if (MWWorld::Class::get (old).getValue (old)>=
                        MWWorld::Class::get (test).getValue (test))
                    {
                        continue;
                    }

                    use = true;
                }
            }

            switch(MWWorld::Class::get (test).canBeEquipped (test, npc).first)
            {
                case 0:
                    continue;
                case 2:
                    invStore.unequipSlot(MWWorld::InventoryStore::Slot_CarriedLeft, npc);
                    break;
                case 3:
                    invStore.unequipSlot(MWWorld::InventoryStore::Slot_CarriedRight, npc);
                    break;
            }

            if (!itemsSlots.second) // if itemsSlots.second is true, item can stay stacked when equipped
            {
                // unstack item pointed to by iterator if required
                if (iter->getRefData().getCount() > 1)
                {
                    // add the item again with a count of count-1, then set the count of the original (that will be equipped) to 1
                    int count = iter->getRefData().getCount();
                    iter->getRefData().setCount(count-1);
                    addNewStack(*iter);
                    iter->getRefData().setCount(1);
                }
            }

            slots[*iter2] = iter;
            break;
        }
    }

    bool changed = false;

    for (std::size_t i=0; i<slots.size(); ++i)
        if (slots[i]!=mSlots[i])
        {
            changed = true;
        }

    mActorModelUpdateEnabled = true;

    if (changed)
    {
        mSlots.swap (slots);
        updateActorModel(npc);
        flagAsModified();
    }
}

const MWMechanics::MagicEffects& MWWorld::InventoryStore::getMagicEffects()
{
    if (!mMagicEffectsUpToDate)
    {
        mMagicEffects = MWMechanics::MagicEffects();

        for (TSlots::const_iterator iter (mSlots.begin()); iter!=mSlots.end(); ++iter)
            if (*iter!=end())
            {
                std::string enchantmentId = MWWorld::Class::get (**iter).getEnchantment (**iter);

                if (!enchantmentId.empty())
                {
                    const ESM::Enchantment& enchantment =
                        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find (enchantmentId);

                    if (enchantment.mData.mType==ESM::Enchantment::ConstantEffect)
                        mMagicEffects.add (enchantment.mEffects);
                }
            }

        mMagicEffectsUpToDate = true;
    }

    return mMagicEffects;
}

void MWWorld::InventoryStore::flagAsModified()
{
    ContainerStore::flagAsModified();
    mMagicEffectsUpToDate = false;
}

bool MWWorld::InventoryStore::stacks(const Ptr& stack, const Ptr& item)
{
    bool canStack = MWWorld::ContainerStore::stacks(stack, item);
    if (!canStack)
        return false;

    // don't stack if 'stack' (the item being checked against) is currently equipped.
    for (TSlots::const_iterator iter (mSlots.begin());
        iter!=mSlots.end(); ++iter)
    {
        if (*iter != end() && stack == **iter)
        {
            bool stackWhenEquipped = MWWorld::Class::get(**iter).getEquipmentSlots(**iter).second;
            if (!stackWhenEquipped)
                return false;
        }
    }

    return true;
}

void MWWorld::InventoryStore::setSelectedEnchantItem(const ContainerStoreIterator& iterator)
{
    mSelectedEnchantItem = iterator;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getSelectedEnchantItem()
{
    return mSelectedEnchantItem;
}

int MWWorld::InventoryStore::remove(const Ptr& item, int count, const Ptr& actor)
{
    for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        if (mSlots[slot] == end())
            continue;

        if (*mSlots[slot] == item)
        {
            // restacking is disabled cause it may break removal
            unequipSlot(slot, actor, false);
            break;
        }
    }

    int retCount = ContainerStore::remove(item, count, actor);

    // If an armor/clothing item is removed, try to find a replacement,
    // but not for the player nor werewolves.
    if ((actor.getRefData().getHandle() != "player")
            && !(MWWorld::Class::get(actor).getNpcStats(actor).isWerewolf()))
    {
        std::string type = item.getTypeName();
        if ((type == typeid(ESM::Armor).name()) || (type == typeid(ESM::Clothing).name()))
            autoEquip(actor);
    }

    return retCount;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipSlot(int slot, const MWWorld::Ptr& actor, bool restack)
{
    ContainerStoreIterator it = getSlot(slot);

    if (it != end())
    {
        ContainerStoreIterator retval = it;

        if (restack) {
            // restack item previously in this slot
            for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
            {
                if (stacks(*iter, *it))
                {
                    iter->getRefData().setCount(iter->getRefData().getCount() + it->getRefData().getCount());
                    it->getRefData().setCount(0);
                    retval = iter;
                    break;
                }
            }
        }

        // empty this slot
        mSlots[slot] = end();

        if (actor.getRefData().getHandle() == "player")
        {
            // Unset OnPCEquip Variable on item's script, if it has a script with that variable declared
            const std::string& script = Class::get(*it).getScript(*it);
            if (script != "")
                (*it).getRefData().getLocals().setVarByInt(script, "onpcequip", 0);

            // Update HUD icon when removing player weapon or selected enchanted item.
            // We have to check for both as the weapon could also be the enchanted item.
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                // weapon
                MWBase::Environment::get().getWindowManager()->unsetSelectedWeapon();
            }
            if ((mSelectedEnchantItem != end()) && (mSelectedEnchantItem == it))
            {
                // enchanted item
                MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();
            }
        }

        updateActorModel(actor);

        return retval;
    }

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipItem(const MWWorld::Ptr& item, const MWWorld::Ptr& actor)
{
    for (int slot=0; slot<MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator equipped = getSlot(slot);
        if (equipped != end() && *equipped == item)
            return unequipSlot(slot, actor);
    }

    throw std::runtime_error ("attempt to unequip an item that is not currently equipped");
}

void MWWorld::InventoryStore::updateActorModel(const MWWorld::Ptr& actor)
{
    if (mActorModelUpdateEnabled)
        MWBase::Environment::get().getWorld()->updateAnimParts(actor);
}
