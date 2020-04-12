#ifndef MWGUI_CONTAINER_ITEM_MODEL_H
#define MWGUI_CONTAINER_ITEM_MODEL_H

#include "itemmodel.hpp"

namespace MWGui
{

    /// @brief The container item model supports multiple item sources, which are needed for
    /// making NPCs sell items from containers owned by them
    class ContainerItemModel : public ItemModel
    {
    public:
        ContainerItemModel (const std::vector<MWWorld::Ptr>& itemSources, const std::vector<MWWorld::Ptr>& worldItems);
        ///< @note The order of elements \a itemSources matters here. The first element has the highest priority for removal,
        ///  while the last element will be used to add new items to.

        ContainerItemModel (const MWWorld::Ptr& source);

        virtual bool allowedToUseItems() const;

        virtual bool onDropItem(const MWWorld::Ptr &item, int count);
        virtual bool onTakeItem(const MWWorld::Ptr &item, int count);

        virtual ItemStack getItem (ModelIndex index);
        virtual ModelIndex getIndex (ItemStack item);
        virtual size_t getItemCount();

        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true);
        virtual void removeItem (const ItemStack& item, size_t count);

        virtual void update();

    private:
        std::vector<MWWorld::Ptr> mItemSources;
        std::vector<MWWorld::Ptr> mWorldItems;

        std::vector<ItemStack> mItems;
    };

}

#endif
