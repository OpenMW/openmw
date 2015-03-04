#include "inventoryitemmodel.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/creaturestats.hpp"

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

MWWorld::Ptr InventoryItemModel::copyItem (const ItemStack& item, size_t count, bool setNewOwner)
{
    if (item.mBase.getContainerStore() == &mActor.getClass().getContainerStore(mActor))
        throw std::runtime_error("Item to copy needs to be from a different container!");
    return *mActor.getClass().getContainerStore(mActor).add(item.mBase, count, mActor, setNewOwner);
}


void InventoryItemModel::removeItem (const ItemStack& item, size_t count)
{
    MWWorld::ContainerStore& store = mActor.getClass().getContainerStore(mActor);
    int removed = store.remove(item.mBase, count, mActor);

    if (removed == 0)
        throw std::runtime_error("Item to remove not found in container store");
    else if (removed < static_cast<int>(count))
        throw std::runtime_error("Not enough items in the stack to remove");
}

MWWorld::Ptr InventoryItemModel::moveItem(const ItemStack &item, size_t count, ItemModel *otherModel)
{
    // Can't move conjured items: This is a general fix that also takes care of issues with taking conjured items via the 'Take All' button.
    if (item.mFlags & ItemStack::Flag_Bound)
        return MWWorld::Ptr();

    MWWorld::Ptr ret = otherModel->copyItem(item, count, false);
    removeItem(item, count);
    return ret;
}

void InventoryItemModel::update()
{
    MWWorld::ContainerStore& store = mActor.getClass().getContainerStore(mActor);

    mItems.clear();

    for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
    {
        MWWorld::Ptr item = *it;
        // NOTE: Don't show WerewolfRobe objects in the inventory, or allow them to be taken.
        // Vanilla likely uses a hack like this since there's no other way to prevent it from
        // being shown or taken.
        if(item.getCellRef().getRefId() == "werewolfrobe")
            continue;

        ItemStack newItem (item, this, item.getRefData().getCount());

        if (mActor.getClass().hasInventoryStore(mActor))
        {
            MWWorld::InventoryStore& store = mActor.getClass().getInventoryStore(mActor);
            if (store.isEquipped(newItem.mBase))
                newItem.mType = ItemStack::Type_Equipped;
        }

        mItems.push_back(newItem);
    }
}

}
