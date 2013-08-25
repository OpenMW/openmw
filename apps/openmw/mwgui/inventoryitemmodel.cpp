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
    if (item.mBase.getContainerStore() == &mActor.getClass().getContainerStore(mActor))
        throw std::runtime_error("Item to copy needs to be from a different container!");
    int origCount = item.mBase.getRefData().getCount();
    item.mBase.getRefData().setCount(count);
    mActor.getClass().getContainerStore(mActor).add(item.mBase, mActor);
    item.mBase.getRefData().setCount(origCount);
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
        MWWorld::Ptr item = *it;
        // NOTE: Don't show WerewolfRobe objects in the inventory, or allow them to be taken.
        // Vanilla likely uses a hack like this since there's no other way to prevent it from
        // being shown or taken.
        if(item.getCellRef().mRefID == "WerewolfRobe")
            continue;

        ItemStack newItem (item, this, item.getRefData().getCount());

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
