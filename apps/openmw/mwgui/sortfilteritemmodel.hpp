#ifndef MWGUI_SORT_FILTER_ITEM_MODEL_H
#define MWGUI_SORT_FILTER_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    class SortFilterItemModel : public ProxyItemModel
    {
    public:
        SortFilterItemModel (ItemModel* sourceModel);

        void update() override;

        bool filterAccepts (const ItemStack& item);

        bool allowedToUseItems() const override;
        ItemStack getItem (ModelIndex index) override;
        size_t getItemCount() override;

        /// Dragged items are not displayed.
        void addDragItem (const MWWorld::Ptr& dragItem, size_t count);
        void clearDragItems();

        void setCategory (int category);
        void setFilter (int filter);
        void setNameFilter (const std::string& filter);
        void setEffectFilter (const std::string& filter);

        /// Use ItemStack::Type for sorting?
        void setSortByType(bool sort) { mSortByType = sort; }

        void onClose() override;
        bool onDropItem(const MWWorld::Ptr &item, int count) override;
        bool onTakeItem(const MWWorld::Ptr &item, int count) override;

        static constexpr int Category_Weapon = (1<<1);
        static constexpr int Category_Apparel = (1<<2);
        static constexpr int Category_Misc = (1<<3);
        static constexpr int Category_Magic = (1<<4);
        static constexpr int Category_All = 255;

        static constexpr int Filter_OnlyIngredients = (1<<0);
        static constexpr int Filter_OnlyEnchanted = (1<<1);
        static constexpr int Filter_OnlyEnchantable = (1<<2);
        static constexpr int Filter_OnlyChargedSoulstones = (1<<3);
        static constexpr int Filter_OnlyUsableItems = (1<<4); // Only items with a Use action
        static constexpr int Filter_OnlyRepairable = (1<<5);
        static constexpr int Filter_OnlyRechargable = (1<<6);
        static constexpr int Filter_OnlyRepairTools = (1<<7);


    private:
        std::vector<ItemStack> mItems;

        std::vector<std::pair<MWWorld::Ptr, size_t> > mDragItems;

        int mCategory;
        int mFilter;
        bool mSortByType;

        std::string mNameFilter; // filter by item name
        std::string mEffectFilter; // filter by magic effect
    };

}

#endif
