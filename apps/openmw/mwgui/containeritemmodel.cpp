#include "containeritemmodel.hpp"

#include <algorithm>

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

namespace
{

    bool stacks(const MWWorld::Ptr& left, const MWWorld::Ptr& right)
    {
        if (left == right)
            return true;

        // If one of the items is in an inventory and currently equipped, we need to check stacking both ways to be sure
        if (left.getContainerStore() && right.getContainerStore())
            return left.getContainerStore()->stacks(left, right) && right.getContainerStore()->stacks(left, right);

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
    ContainerItemModel::ContainerItemModel(
        const std::vector<MWWorld::Ptr>& itemSources, const std::vector<MWWorld::Ptr>& worldItems)
        : mWorldItems(worldItems)
        , mTrading(true)
    {
        assert(!itemSources.empty());
        // Tie resolution lifetimes to the ItemModel
        mItemSources.reserve(itemSources.size());
        for (const MWWorld::Ptr& source : itemSources)
        {
            MWWorld::ContainerStore& store = source.getClass().getContainerStore(source);
            mItemSources.emplace_back(source, store.resolveTemporarily());
        }
    }

    ContainerItemModel::ContainerItemModel(const MWWorld::Ptr& source)
        : mTrading(false)
    {
        MWWorld::ContainerStore& store = source.getClass().getContainerStore(source);
        mItemSources.emplace_back(source, store.resolveTemporarily());
    }

    bool ContainerItemModel::allowedToUseItems() const
    {
        if (mItemSources.empty())
            return true;

        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        MWWorld::Ptr victim;

        // Check if the player is allowed to use items from opened container
        MWBase::MechanicsManager* mm = MWBase::Environment::get().getMechanicsManager();
        return mm->isAllowedToUse(ptr, mItemSources[0].first, victim);
    }

    ItemStack ContainerItemModel::getItem(ModelIndex index)
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

    ItemModel::ModelIndex ContainerItemModel::getIndex(const ItemStack& item)
    {
        ModelIndex i = 0;
        for (ItemStack& itemStack : mItems)
        {
            if (itemStack == item)
                return i;
            ++i;
        }
        return -1;
    }

    MWWorld::Ptr ContainerItemModel::addItem(const ItemStack& item, size_t count, bool allowAutoEquip)
    {
        auto& source = mItemSources[0];
        MWWorld::ContainerStore& store = source.first.getClass().getContainerStore(source.first);
        if (item.mBase.getContainerStore() == &store)
            throw std::runtime_error("Item to add needs to be from a different container!");
        return *store.add(item.mBase, static_cast<int>(count), allowAutoEquip);
    }

    MWWorld::Ptr ContainerItemModel::copyItem(const ItemStack& item, size_t count, bool allowAutoEquip)
    {
        auto& source = mItemSources[0];
        MWWorld::ContainerStore& store = source.first.getClass().getContainerStore(source.first);
        if (item.mBase.getContainerStore() == &store)
            throw std::runtime_error("Item to copy needs to be from a different container!");
        MWWorld::ManualRef newRef(*MWBase::Environment::get().getESMStore(), item.mBase, static_cast<int>(count));
        return *store.add(newRef.getPtr(), static_cast<int>(count), allowAutoEquip);
    }

    void ContainerItemModel::removeItem(const ItemStack& item, size_t count)
    {
        int toRemove = static_cast<int>(count);

        for (auto& source : mItemSources)
        {
            MWWorld::ContainerStore& store = source.first.getClass().getContainerStore(source.first);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                if (stacks(*it, item.mBase))
                {
                    int quantity = it->mRef->mRef.getCount(false);
                    // If this is a restocking quantity, just don't remove it
                    if (quantity < 0 && mTrading)
                        toRemove += quantity;
                    else
                        toRemove -= store.remove(*it, toRemove);
                    if (toRemove <= 0)
                        return;
                }
            }
        }
        for (MWWorld::Ptr& source : mWorldItems)
        {
            if (stacks(source, item.mBase))
            {
                int refCount = source.getCellRef().getCount();
                if (refCount - toRemove <= 0)
                    MWBase::Environment::get().getWorld()->deleteObject(source);
                else
                    source.getCellRef().setCount(std::max(0, refCount - toRemove));
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
        for (auto& source : mItemSources)
        {
            MWWorld::ContainerStore& store = source.first.getClass().getContainerStore(source.first);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                if (!(*it).getClass().showsInInventory(*it))
                    continue;

                bool found = false;
                for (ItemStack& itemStack : mItems)
                {
                    if (stacks(*it, itemStack.mBase))
                    {
                        // we already have an item stack of this kind, add to it
                        itemStack.mCount += it->getCellRef().getCount();
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    // no stack yet, create one
                    ItemStack newItem(*it, this, it->getCellRef().getCount());
                    mItems.push_back(newItem);
                }
            }
        }
        for (MWWorld::Ptr& source : mWorldItems)
        {
            bool found = false;
            for (ItemStack& itemStack : mItems)
            {
                if (stacks(source, itemStack.mBase))
                {
                    // we already have an item stack of this kind, add to it
                    itemStack.mCount += source.getCellRef().getCount();
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                // no stack yet, create one
                ItemStack newItem(source, this, source.getCellRef().getCount());
                mItems.push_back(newItem);
            }
        }
    }
    bool ContainerItemModel::onDropItem(const MWWorld::Ptr& item, int count)
    {
        if (mItemSources.empty())
            return false;

        MWWorld::Ptr target = mItemSources[0].first;

        if (target.getType() != ESM::Container::sRecordId)
            return true;

        // Check container organic flag
        MWWorld::LiveCellRef<ESM::Container>* ref = target.get<ESM::Container>();
        if (ref->mBase->mFlags & ESM::Container::Organic)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage2}");
            return false;
        }

        // Check for container without capacity
        float capacity = target.getClass().getCapacity(target);
        if (capacity <= 0.0f)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage3}");
            return false;
        }

        // Check the container capacity plus one increment so the expected total weight can
        // fit in the container with floating-point imprecision
        float newEncumbrance = target.getClass().getEncumbrance(target) + (item.getClass().getWeight(item) * count);
        if (std::nextafterf(capacity, std::numeric_limits<float>::max()) < newEncumbrance)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage3}");
            return false;
        }

        return true;
    }

    bool ContainerItemModel::onTakeItem(const MWWorld::Ptr& item, int count)
    {
        if (mItemSources.empty())
            return false;

        MWWorld::Ptr target = mItemSources[0].first;

        // Looting a dead corpse is considered OK
        if (target.getClass().isActor() && target.getClass().getCreatureStats(target).isDead())
            return true;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWBase::Environment::get().getMechanicsManager()->itemTaken(player, item, target, count);

        return true;
    }

    bool ContainerItemModel::usesContainer(const MWWorld::Ptr& container)
    {
        for (const auto& source : mItemSources)
        {
            if (source.first == container)
                return true;
        }
        return false;
    }

}
