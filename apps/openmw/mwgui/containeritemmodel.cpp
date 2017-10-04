#include "containeritemmodel.hpp"

#include <algorithm>

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace
{

    bool stacks (const MWWorld::Ptr& left, const MWWorld::Ptr& right)
    {
        if (left == right)
            return true;

        // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
        if (left.getContainerStore() && right.getContainerStore())
            return left.getContainerStore()->stacks(left, right)
                    && right.getContainerStore()->stacks(left, right);

        if (left.getContainerStore())
            return left.getContainerStore()->stacks(left, right);
        if (right.getContainerStore())
            return right.getContainerStore()->stacks(left, right);

        MWWorld::ContainerStore store;
        return store.stacks(left, right);
    }

}

namespace MWGui
{

ContainerItemModel::ContainerItemModel(const std::vector<MWWorld::Ptr>& itemSources, const std::vector<MWWorld::Ptr>& worldItems)
    : mItemSources(itemSources)
    , mWorldItems(worldItems)
{
    assert (!mItemSources.empty());
}

ContainerItemModel::ContainerItemModel (const MWWorld::Ptr& source)
{
    mItemSources.push_back(source);
}

bool ContainerItemModel::allowedToUseItems() const
{
    if (mItemSources.size() == 0)
        return true;

    MWWorld::Ptr ptr = MWMechanics::getPlayer();
    MWWorld::Ptr victim;

    // Check if the player is allowed to use items from opened container
    MWBase::MechanicsManager* mm = MWBase::Environment::get().getMechanicsManager();
    return mm->isAllowedToUse(ptr, mItemSources[0], victim);
}

ItemStack ContainerItemModel::getItem (ModelIndex index)
{
    if (index < 0)
        throw std::runtime_error("Invalid index supplied");
    if (mItems.size() <= static_cast<size_t>(index))
        throw std::runtime_error("Item index out of range");
    return mItems[index];
}

size_t ContainerItemModel::getItemCount()
{
    return mItems.size();
}

ItemModel::ModelIndex ContainerItemModel::getIndex (ItemStack item)
{
    size_t i = 0;
    for (std::vector<ItemStack>::iterator it = mItems.begin(); it != mItems.end(); ++it)
    {
        if (*it == item)
            return i;
        ++i;
    }
    return -1;
}

MWWorld::Ptr ContainerItemModel::copyItem (const ItemStack& item, size_t count, bool setNewOwner)
{
    const MWWorld::Ptr& source = mItemSources[mItemSources.size()-1];
    if (item.mBase.getContainerStore() == &source.getClass().getContainerStore(source))
        throw std::runtime_error("Item to copy needs to be from a different container!");
    return *source.getClass().getContainerStore(source).add(item.mBase, count, source);
}

void ContainerItemModel::removeItem (const ItemStack& item, size_t count)
{
    int toRemove = count;

    for (std::vector<MWWorld::Ptr>::iterator source = mItemSources.begin(); source != mItemSources.end(); ++source)
    {
        MWWorld::ContainerStore& store = source->getClass().getContainerStore(*source);

        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            if (stacks(*it, item.mBase))
            {
                toRemove -= store.remove(*it, toRemove, *source);
                if (toRemove <= 0)
                    return;
            }
        }
    }
    for (std::vector<MWWorld::Ptr>::iterator source = mWorldItems.begin(); source != mWorldItems.end(); ++source)
    {
        if (stacks(*source, item.mBase))
        {
            int refCount = source->getRefData().getCount();
            if (refCount - toRemove <= 0)
                MWBase::Environment::get().getWorld()->deleteObject(*source);
            else
                source->getRefData().setCount(std::max(0, refCount - toRemove));
            toRemove -= refCount;
            if (toRemove <= 0)
                return;
        }
    }

    throw std::runtime_error("Not enough items to remove could be found");
}

void ContainerItemModel::update()
{
    mItems.clear();
    for (std::vector<MWWorld::Ptr>::iterator source = mItemSources.begin(); source != mItemSources.end(); ++source)
    {
        MWWorld::ContainerStore& store = source->getClass().getContainerStore(*source);

        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            std::vector<ItemStack>::iterator itemStack = mItems.begin();
            for (; itemStack != mItems.end(); ++itemStack)
            {
                if (stacks(*it, itemStack->mBase))
                {
                    // we already have an item stack of this kind, add to it
                    itemStack->mCount += it->getRefData().getCount();
                    break;
                }
            }

            if (itemStack == mItems.end())
            {
                // no stack yet, create one
                ItemStack newItem (*it, this, it->getRefData().getCount());
                mItems.push_back(newItem);
            }
        }
    }
    for (std::vector<MWWorld::Ptr>::iterator source = mWorldItems.begin(); source != mWorldItems.end(); ++source)
    {
        std::vector<ItemStack>::iterator itemStack = mItems.begin();
        for (; itemStack != mItems.end(); ++itemStack)
        {
            if (stacks(*source, itemStack->mBase))
            {
                // we already have an item stack of this kind, add to it
                itemStack->mCount += source->getRefData().getCount();
                break;
            }
        }

        if (itemStack == mItems.end())
        {
            // no stack yet, create one
            ItemStack newItem (*source, this, source->getRefData().getCount());
            mItems.push_back(newItem);
        }
    }
}

}
