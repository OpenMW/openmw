#include "tradeitemmodel.hpp"

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
        std::vector<ItemStack>::iterator it = out.begin();
        bool found = false;
        for (; it != out.end(); ++it)
        {
            if (it->stacks(item))
            {
                it->mCount += item.mCount;
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
            if (it->stacks(item))
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
        std::vector<ItemStack>::iterator it = mBorrowedToUs.begin();
        for (; it != mBorrowedToUs.end(); ++it)
        {
            // get index in the source model
            ItemModel* sourceModel = it->mCreator;
            size_t i=0;
            for (; i<sourceModel->getItemCount(); ++i)
            {
                if (it->stacks(sourceModel->getItem(i)))
                    break;
            }
            if (i == sourceModel->getItemCount())
                throw std::runtime_error("The borrowed item disappeared");

            // reset owner before copying
            const ItemStack& item = sourceModel->getItem(i);
            std::string owner = item.mBase.getCellRef().mOwner;
            if (mMerchant.isEmpty()) // only for items bought by player
                item.mBase.getCellRef().mOwner = "";
            // copy the borrowed items to our model
            copyItem(item, it->mCount);
            item.mBase.getCellRef().mOwner = owner;
            // then remove them from the source model
            sourceModel->removeItem(item, it->mCount);
        }
        mBorrowedToUs.clear();
        mBorrowedFromUs.clear();
    }

    void TradeItemModel::update()
    {
        mSourceModel->update();

        int services = 0;
        if (!mMerchant.isEmpty())
            services = MWWorld::Class::get(mMerchant).getServices(mMerchant);

        mItems.clear();
        // add regular items
        for (size_t i=0; i<mSourceModel->getItemCount(); ++i)
        {
            ItemStack item = mSourceModel->getItem(i);
            MWWorld::Ptr base = item.mBase;
            if (!mMerchant.isEmpty() && Misc::StringUtils::ciEqual(base.getCellRef().mRefID, "gold_001"))
                continue;
            if (!mMerchant.isEmpty() && !MWWorld::Class::get(base).canSell(base, services))
                continue;

            // don't show equipped items
            if (mMerchant.getTypeName() == typeid(ESM::NPC).name())
            {
                bool isEquipped = false;
                MWWorld::InventoryStore& store = MWWorld::Class::get(mMerchant).getInventoryStore(mMerchant);
                for (int slot=0; slot<MWWorld::InventoryStore::Slots; ++slot)
                {
                    MWWorld::ContainerStoreIterator equipped = store.getSlot(slot);
                    if (equipped == store.end())
                        continue;
                    if (*equipped == base)
                        isEquipped = true;
                }
                if (isEquipped)
                    continue;
            }

            // don't show items that we borrowed to someone else
            std::vector<ItemStack>::iterator it = mBorrowedFromUs.begin();
            for (; it != mBorrowedFromUs.end(); ++it)
            {
                if (it->stacks(item))
                {
                    if (item.mCount < it->mCount)
                        throw std::runtime_error("Lent more items than present");
                    item.mCount -= it->mCount;
                }
            }

            if (item.mCount > 0)
                mItems.push_back(item);
        }

        // add items borrowed to us
        std::vector<ItemStack>::iterator it = mBorrowedToUs.begin();
        for (; it != mBorrowedToUs.end(); ++it)
        {
            ItemStack item = *it;
            item.mType = ItemStack::Type_Barter;
            mItems.push_back(item);
        }
    }

}
