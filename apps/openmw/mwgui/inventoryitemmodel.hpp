#ifndef MWGUI_INVENTORY_ITEM_MODEL_H
#define MWGUI_INVENTORY_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    class InventoryItemModel : public ItemModel
    {
    public:
        InventoryItemModel (const MWWorld::Ptr& actor);

        virtual ItemStack getItem (ModelIndex index);
        virtual ModelIndex getIndex (ItemStack item);
        virtual size_t getItemCount();

        virtual void copyItem (const ItemStack& item, size_t count);
        virtual void removeItem (const ItemStack& item, size_t count);

        virtual void update();

    protected:
        MWWorld::Ptr mActor;
    private:
        std::vector<ItemStack> mItems;
    };

}

#endif
