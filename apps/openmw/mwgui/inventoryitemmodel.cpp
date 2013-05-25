#include "inventoryitemmodel.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWGui
{

InventoryItemModel::InventoryItemModel(const MWWorld::Ptr &actor)
    : mActor(actor)
{
}

ItemStack InventoryItemModel::getItem (ModelIndex index)
{
    if (index < 0)
        throw std::runtime_error("Invalid index supplied");
    if (mItems.size() <= static_cast<size_t>(index))
        throw std::runtime_error("Item index out of range");
    return mItems[index];
}

size_t InventoryItemModel::getItemCount()
{
    return mItems.size();
}

ItemModel::ModelIndex InventoryItemModel::getIndex (ItemStack item)
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

void InventoryItemModel::copyItem (const ItemStack& item, size_t count)
{
    int origCount = item.mBase.getRefData().getCount();
    item.mBase.getRefData().setCount(count);
    MWWorld::ContainerStoreIterator it = MWWorld::Class::get(mActor).getContainerStore(mActor).add(item.mBase);
    if (*it != item.mBase)
        item.mBase.getRefData().setCount(origCount);
    else
        item.mBase.getRefData().setCount(origCount + count); // item copied onto itself
}


void InventoryItemModel::removeItem (const ItemStack& item, size_t count)
{
    MWWorld::ContainerStore& store = MWWorld::Class::get(mActor).getContainerStore(mActor);

    for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
    {
        if (*it == item.mBase)
        {
            if (it->getRefData().getCount() < static_cast<int>(count))
                throw std::runtime_error("Not enough items in the stack to remove");
            it->getRefData().setCount(it->getRefData().getCount() - count);
            return;
        }
    }
    throw std::runtime_error("Item to remove not found in container store");
}

void InventoryItemModel::update()
{
    MWWorld::ContainerStore& store = MWWorld::Class::get(mActor).getContainerStore(mActor);

    mItems.clear();

    for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
    {
        ItemStack newItem (*it, this, it->getRefData().getCount());

        if (mActor.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::InventoryStore& store = MWWorld::Class::get(mActor).getInventoryStore(mActor);
            for (int slot=0; slot<MWWorld::InventoryStore::Slots; ++slot)
            {
                MWWorld::ContainerStoreIterator equipped = store.getSlot(slot);
                if (equipped == store.end())
                    continue;
                if (*equipped == newItem.mBase)
                    newItem.mType = ItemStack::Type_Equipped;
            }
        }

        mItems.push_back(newItem);
    }
}

}
