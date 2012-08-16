
#include "inventorystore.hpp"

#include <iterator>
#include <algorithm>

#include <components/esm/loadench.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"

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

void MWWorld::InventoryStore::equip (int slot, const ContainerStoreIterator& iterator)
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

    // restack item previously in this slot (if required)
    if (mSlots[slot] != end())
    {
        for (MWWorld::ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
        {
            if (stacks(*iter, *mSlots[slot]))
            {
                iter->getRefData().setCount( iter->getRefData().getCount() + mSlots[slot]->getRefData().getCount() );
                mSlots[slot]->getRefData().setCount(0);
                break;
            }
        }
    }

    // unstack item pointed to by iterator if required
    if (iterator!=end() && !slots.second && iterator->getRefData().getCount() > 1) // if slots.second is true, item can stay stacked when equipped
    {
        // add the item again with a count of count-1, then set the count of the original (that will be equipped) to 1
        int count = iterator->getRefData().getCount();
        iterator->getRefData().setCount(count-1);
        addImpl(*iterator);
        iterator->getRefData().setCount(1);
    }

    mSlots[slot] = iterator;

    flagAsModified();
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

void MWWorld::InventoryStore::autoEquip (const MWMechanics::NpcStats& stats)
{
    TSlots slots;
    initSlots (slots);

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

            if (!itemsSlots.second) // if itemsSlots.second is true, item can stay stacked when equipped
            {
                // unstack item pointed to by iterator if required
                if (iter->getRefData().getCount() > 1)
                {
                    // add the item again with a count of count-1, then set the count of the original (that will be equipped) to 1
                    int count = iter->getRefData().getCount();
                    iter->getRefData().setCount(count-1);
                    addImpl(*iter);
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

    if (changed)
    {
        mSlots.swap (slots);
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
                        *MWBase::Environment::get().getWorld()->getStore().enchants.find (enchantmentId);

                    if (enchantment.data.type==ESM::Enchantment::ConstantEffect)
                        mMagicEffects.add (enchantment.effects);
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

bool MWWorld::InventoryStore::stacks(const Ptr& ptr1, const Ptr& ptr2)
{
    bool canStack = MWWorld::ContainerStore::stacks(ptr1, ptr2);
    if (!canStack)
        return false;

    // don't stack if the item being checked against is currently equipped.
    for (TSlots::const_iterator iter (mSlots.begin());
        iter!=mSlots.end(); ++iter)
    {
        if (*iter != end() && ptr1 == **iter)
            return false;
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
