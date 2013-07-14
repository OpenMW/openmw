#ifndef MWGUI_SORT_FILTER_ITEM_MODEL_H
#define MWGUI_SORT_FILTER_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    class SortFilterItemModel : public ProxyItemModel
    {
    public:
        SortFilterItemModel (ItemModel* sourceModel);

        virtual void update();

        bool filterAccepts (const ItemStack& item);

        virtual ItemStack getItem (ModelIndex index);
        virtual size_t getItemCount();

        /// Dragged items are not displayed.
        void addDragItem (const MWWorld::Ptr& dragItem, size_t count);
        void clearDragItems();

        void setCategory (int category);
        void setFilter (int filter);
        void setShowEquipped (bool show) { mShowEquipped = show; }

        static const int Category_Weapon = (1<<1);
        static const int Category_Apparel = (1<<2);
        static const int Category_Misc = (1<<3);
        static const int Category_Magic = (1<<4);
        static const int Category_All = 255;

        static const int Filter_OnlyIngredients = (1<<0);
        static const int Filter_OnlyEnchanted = (1<<1);
        static const int Filter_OnlyEnchantable = (1<<2);
        static const int Filter_OnlyChargedSoulstones = (1<<3);


    private:
        std::vector<ItemStack> mItems;

        std::vector<std::pair<MWWorld::Ptr, size_t> > mDragItems;

        int mCategory;
        int mFilter;
        bool mShowEquipped;
    };

}

#endif
