#ifndef MWGUI_INVENTORY_ITEM_MODEL_H
#define MWGUI_INVENTORY_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    class InventoryItemModel : public ItemModel
    {
    public:
        InventoryItemModel (const MWWorld::Ptr& actor);

        ItemStack getItem (ModelIndex index) override;
        ModelIndex getIndex (ItemStack item) override;
        size_t getItemCount() override;

        bool onTakeItem(const MWWorld::Ptr &item, int count) override;

        MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true) override;
        void removeItem (const ItemStack& item, size_t count) override;

        /// Move items from this model to \a otherModel.
        MWWorld::Ptr moveItem (const ItemStack& item, size_t count, ItemModel* otherModel) override;

        void update() override;

    protected:
        MWWorld::Ptr mActor;
    private:
        std::vector<ItemStack> mItems;
    };

}

#endif
