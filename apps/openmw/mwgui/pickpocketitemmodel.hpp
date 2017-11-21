#ifndef MWGUI_PICKPOCKET_ITEM_MODEL_H
#define MWGUI_PICKPOCKET_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    /// @brief The pickpocket item model randomly hides item stacks based on a specified chance. Equipped items are always hidden.
    class PickpocketItemModel : public ProxyItemModel
    {
    public:
        PickpocketItemModel (const MWWorld::Ptr& thief, ItemModel* sourceModel, bool hideItems=true);

        virtual bool allowedToUseItems() const;
        virtual ItemStack getItem (ModelIndex index);
        virtual size_t getItemCount();
        virtual void update();
        virtual void removeItem (const ItemStack& item, size_t count);
        virtual void onClose();
        virtual bool onDropItem(const MWWorld::Ptr &item, int count);
        virtual bool onTakeItem(const MWWorld::Ptr &item, int count);

    protected:
        MWWorld::Ptr mActor;
        bool mPickpocketDetected;
        bool stealItem(const MWWorld::Ptr &item, int count);

    private:
        std::vector<ItemStack> mHiddenItems;
        std::vector<ItemStack> mItems;
    };

}

#endif
