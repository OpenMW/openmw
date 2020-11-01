#ifndef MWGUI_TRADE_ITEM_MODEL_H
#define MWGUI_TRADE_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    class ItemModel;

    /// @brief An item model that allows 'borrowing' items from another item model. Used for previewing barter offers.
    /// Also filters items that the merchant does not sell.
    class TradeItemModel : public ProxyItemModel
    {
    public:
        TradeItemModel (ItemModel* sourceModel, const MWWorld::Ptr& merchant);

        bool allowedToUseItems() const override;

        ItemStack getItem (ModelIndex index) override;
        size_t getItemCount() override;

        void update() override;

        void borrowItemFromUs (ModelIndex itemIndex, size_t count);

        void borrowItemToUs (ModelIndex itemIndex, ItemModel* source, size_t count);
        ///< @note itemIndex points to an item in \a source

        void returnItemBorrowedToUs (ModelIndex itemIndex, size_t count);

        void returnItemBorrowedFromUs (ModelIndex itemIndex, ItemModel* source, size_t count);

        /// Permanently transfers items that were borrowed to us from another model to this model
        void transferItems ();
        /// Aborts trade
        void abort();

        /// Adjusts the given encumbrance by adding weight for items that have been lent to us,
        /// and removing weight for items we've lent to someone else.
        void adjustEncumbrance (float& encumbrance);

        const std::vector<ItemStack> getItemsBorrowedToUs() const;

    private:
        void borrowImpl(const ItemStack& item, std::vector<ItemStack>& out);
        void unborrowImpl(const ItemStack& item, size_t count, std::vector<ItemStack>& out);

        std::vector<ItemStack> mItems;

        std::vector<ItemStack> mBorrowedToUs;
        std::vector<ItemStack> mBorrowedFromUs;

        MWWorld::Ptr mMerchant;
    };

}

#endif
