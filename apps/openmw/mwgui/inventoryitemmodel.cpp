#include "inventoryitemmodel.hpp"

#include <sstream>

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

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
    for (ItemStack& itemStack : mItems)
    {
        if (itemStack == item)
            return i;
        ++i;
    }
    return -1;
}

MWWorld::Ptr InventoryItemModel::copyItem (const ItemStack& item, size_t count, bool allowAutoEquip)
{
    if (item.mBase.getContainerStore() == &mActor.getClass().getContainerStore(mActor))
        throw std::runtime_error("Item to copy needs to be from a different container!");
    return *mActor.getClass().getContainerStore(mActor).add(item.mBase, count, mActor, allowAutoEquip);
}

void InventoryItemModel::removeItem (const ItemStack& item, size_t count)
{
    int removed = 0;
    // Re-equipping makes sense only if a target has inventory
    if (mActor.getClass().hasInventoryStore(mActor))
    {
        MWWorld::InventoryStore& store = mActor.getClass().getInventoryStore(mActor);
        removed = store.remove(item.mBase, count, mActor, true);
    }
    else
    {
        MWWorld::ContainerStore& store = mActor.getClass().getContainerStore(mActor);
        removed = store.remove(item.mBase, count, mActor);
    }

    std::stringstream error;

    if (removed == 0)
    {
        error << "Item '" << item.mBase.getCellRef().getRefId() << "' was not found in container store to remove";
        throw std::runtime_error(error.str());
    }
    else if (removed < static_cast<int>(count))
    {
        error << "Not enough items '" << item.mBase.getCellRef().getRefId() << "' in the stack to remove (" << static_cast<int>(count) << " requested, " << removed << " found)";
        throw std::runtime_error(error.str());
    }
}

MWWorld::Ptr InventoryItemModel::moveItem(const ItemStack &item, size_t count, ItemModel *otherModel)
{
    // Can't move conjured items: This is a general fix that also takes care of issues with taking conjured items via the 'Take All' button.
    if (item.mFlags & ItemStack::Flag_Bound)
        return MWWorld::Ptr();

    MWWorld::Ptr ret = otherModel->copyItem(item, count);
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

        if (!item.getClass().showsInInventory(item))
            continue;

        ItemStack newItem (item, this, item.getRefData().getCount());

        if (mActor.getClass().hasInventoryStore(mActor))
        {
            MWWorld::InventoryStore& invStore = mActor.getClass().getInventoryStore(mActor);
            if (invStore.isEquipped(newItem.mBase))
                newItem.mType = ItemStack::Type_Equipped;
        }

        mItems.push_back(newItem);
    }
}

bool InventoryItemModel::onTakeItem(const MWWorld::Ptr &item, int count)
{
    // Looting a dead corpse is considered OK
    if (mActor.getClass().isActor() && mActor.getClass().getCreatureStats(mActor).isDead())
        return true;

    MWWorld::Ptr player = MWMechanics::getPlayer();
    MWBase::Environment::get().getMechanicsManager()->itemTaken(player, item, mActor, count);

    return true;
}

}
