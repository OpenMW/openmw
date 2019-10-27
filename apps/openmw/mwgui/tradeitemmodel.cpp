#include "tradeitemmodel.hpp"

#include <components/misc/stringops.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWGui
{

    TradeItemModel::TradeItemModel(ItemModel *sourceModel, const MWWorld::Ptr& merchant)
        : mMerchant(merchant)
    {
        mSourceModel = sourceModel;
    }

    bool TradeItemModel::allowedToUseItems() const
    {
        return true;
    }

    ItemStack TradeItemModel::getItem (ModelIndex index)
    {
        if (index < 0)
            throw std::runtime_error("Invalid index supplied");
        if (mItems.size() <= static_cast<size_t>(index))
            throw std::runtime_error("Item index out of range");
        return mItems[index];
    }

    size_t TradeItemModel::getItemCount()
    {
        return mItems.size();
    }

    void TradeItemModel::borrowImpl(const ItemStack &item, std::vector<ItemStack> &out)
    {
        bool found = false;
        for (ItemStack& itemStack : out)
        {
            if (itemStack.mBase == item.mBase)
            {
                itemStack.mCount += item.mCount;
                found = true;
                break;
            }
        }
        if (!found)
            out.push_back(item);
    }

    void TradeItemModel::unborrowImpl(const ItemStack &item, size_t count, std::vector<ItemStack> &out)
    {
        std::vector<ItemStack>::iterator it = out.begin();
        bool found = false;
        for (; it != out.end(); ++it)
        {
            if (it->mBase == item.mBase)
            {
                if (it->mCount < count)
                    throw std::runtime_error("Not enough borrowed items to return");
                it->mCount -= count;
                if (it->mCount == 0)
                    out.erase(it);
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error("Can't find borrowed item to return");
    }

    void TradeItemModel::borrowItemFromUs (ModelIndex itemIndex, size_t count)
    {
        ItemStack item = getItem(itemIndex);
        item.mCount = count;
        borrowImpl(item, mBorrowedFromUs);
    }

    void TradeItemModel::borrowItemToUs (ModelIndex itemIndex, ItemModel* source, size_t count)
    {
        ItemStack item = source->getItem(itemIndex);
        item.mCount = count;
        borrowImpl(item, mBorrowedToUs);
    }

    void TradeItemModel::returnItemBorrowedToUs (ModelIndex itemIndex, size_t count)
    {
        ItemStack item = getItem(itemIndex);
        unborrowImpl(item, count, mBorrowedToUs);
    }

    void TradeItemModel::returnItemBorrowedFromUs (ModelIndex itemIndex, ItemModel* source, size_t count)
    {
        ItemStack item = source->getItem(itemIndex);
        unborrowImpl(item, count, mBorrowedFromUs);
    }

    void TradeItemModel::adjustEncumbrance(float &encumbrance)
    {
        for (ItemStack& itemStack : mBorrowedToUs)
        {
            MWWorld::Ptr& item = itemStack.mBase;
            encumbrance += item.getClass().getWeight(item) * itemStack.mCount;
        }
        for (ItemStack& itemStack : mBorrowedFromUs)
        {
            MWWorld::Ptr& item = itemStack.mBase;
            encumbrance -= item.getClass().getWeight(item) * itemStack.mCount;
        }
        encumbrance = std::max(0.f, encumbrance);
    }

    void TradeItemModel::abort()
    {
        mBorrowedFromUs.clear();
        mBorrowedToUs.clear();
    }

    std::vector<ItemStack> TradeItemModel::getItemsBorrowedToUs()
    {
        return mBorrowedToUs;
    }

    void TradeItemModel::transferItems()
    {
        for (ItemStack& itemStack : mBorrowedToUs)
        {
            // get index in the source model
            ItemModel* sourceModel = itemStack.mCreator;
            size_t i=0;
            for (; i<sourceModel->getItemCount(); ++i)
            {
                if (itemStack.mBase == sourceModel->getItem(i).mBase)
                    break;
            }
            if (i == sourceModel->getItemCount())
                throw std::runtime_error("The borrowed item disappeared");

            const ItemStack& item = sourceModel->getItem(i);
            static const bool prevent = Settings::Manager::getBool("prevent merchant equipping", "Game");
            // copy the borrowed items to our model
            copyItem(item, itemStack.mCount, !prevent);
            // then remove them from the source model
            sourceModel->removeItem(item, itemStack.mCount);
        }
        mBorrowedToUs.clear();
        mBorrowedFromUs.clear();
    }

    void TradeItemModel::update()
    {
        mSourceModel->update();

        int services = 0;
        if (!mMerchant.isEmpty())
            services = mMerchant.getClass().getServices(mMerchant);

        mItems.clear();
        // add regular items
        for (size_t i=0; i<mSourceModel->getItemCount(); ++i)
        {
            ItemStack item = mSourceModel->getItem(i);
            if(!mMerchant.isEmpty())
            {
                MWWorld::Ptr base = item.mBase;
                if(Misc::StringUtils::ciEqual(base.getCellRef().getRefId(), MWWorld::ContainerStore::sGoldId))
                    continue;

                if (!base.getClass().showsInInventory(base))
                    return;

                if(!base.getClass().canSell(base, services))
                    continue;

                // Bound items may not be bought
                if (item.mFlags & ItemStack::Flag_Bound)
                    continue;

                // don't show equipped items
                if(mMerchant.getClass().hasInventoryStore(mMerchant))
                {
                    MWWorld::InventoryStore& store = mMerchant.getClass().getInventoryStore(mMerchant);
                    if (store.isEquipped(base))
                        continue;
                }
            }

            // don't show items that we borrowed to someone else
            for (ItemStack& itemStack : mBorrowedFromUs)
            {
                if (itemStack.mBase == item.mBase)
                {
                    if (item.mCount < itemStack.mCount)
                        throw std::runtime_error("Lent more items than present");
                    item.mCount -= itemStack.mCount;
                }
            }

            if (item.mCount > 0)
                mItems.push_back(item);
        }

        // add items borrowed to us
        for (ItemStack& itemStack : mBorrowedToUs)
        {
            itemStack.mType = ItemStack::Type_Barter;
            mItems.push_back(itemStack);
        }
    }

}
