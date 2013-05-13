#include "containeritemmodel.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"

namespace MWGui
{

ContainerItemModel::ContainerItemModel(const std::vector<MWWorld::Ptr>& itemSources)
    : mItemSources(itemSources)
{
    assert (mItemSources.size());
}

ContainerItemModel::ContainerItemModel (const MWWorld::Ptr& source)
{
    mItemSources.push_back(source);
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

void ContainerItemModel::copyItem (const ItemStack& item, size_t count)
{
    const MWWorld::Ptr& source = mItemSources[mItemSources.size()-1];
    int origCount = item.mBase.getRefData().getCount();
    item.mBase.getRefData().setCount(count);
    MWWorld::ContainerStoreIterator it = MWWorld::Class::get(source).getContainerStore(source).add(item.mBase);
    if (*it != item.mBase)
        item.mBase.getRefData().setCount(origCount);
    else
        item.mBase.getRefData().setCount(origCount + count); // item copied onto itself
}

void ContainerItemModel::removeItem (const ItemStack& item, size_t count)
{
    int toRemove = count;

    for (std::vector<MWWorld::Ptr>::iterator source = mItemSources.begin(); source != mItemSources.end(); ++source)
    {
        MWWorld::ContainerStore& store = MWWorld::Class::get(*source).getContainerStore(*source);

        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
            if (*it == item.mBase || (store.stacks(*it, item.mBase) && item.mBase.getContainerStore()->stacks(*it, item.mBase)))
            {
                int refCount = it->getRefData().getCount();
                it->getRefData().setCount(std::max(0, refCount - toRemove));
                toRemove -= refCount;
                if (toRemove <= 0)
                    return;
            }
        }
    }
    throw std::runtime_error("Not enough items to remove could be found");
}

void ContainerItemModel::update()
{
    mItems.clear();
    for (std::vector<MWWorld::Ptr>::iterator source = mItemSources.begin(); source != mItemSources.end(); ++source)
    {
        MWWorld::ContainerStore& store = MWWorld::Class::get(*source).getContainerStore(*source);

        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            std::vector<ItemStack>::iterator itemStack = mItems.begin();
            for (; itemStack != mItems.end(); ++itemStack)
            {
                // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
                if (store.stacks(itemStack->mBase, *it) && it->getContainerStore()->stacks(itemStack->mBase, *it))
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
}

}
