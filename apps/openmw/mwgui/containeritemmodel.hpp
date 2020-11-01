#ifndef MWGUI_CONTAINER_ITEM_MODEL_H
#define MWGUI_CONTAINER_ITEM_MODEL_H

#include <utility>
#include <vector>

#include "itemmodel.hpp"

#include "../mwworld/containerstore.hpp"

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

        bool allowedToUseItems() const override;

        bool onDropItem(const MWWorld::Ptr &item, int count) override;
        bool onTakeItem(const MWWorld::Ptr &item, int count) override;

        ItemStack getItem (ModelIndex index) override;
        ModelIndex getIndex (ItemStack item) override;
        size_t getItemCount() override;

        MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true) override;
        void removeItem (const ItemStack& item, size_t count) override;

        void update() override;

    private:
        std::vector<std::pair<MWWorld::Ptr, MWWorld::ResolutionHandle>> mItemSources;
        std::vector<MWWorld::Ptr> mWorldItems;
        const bool mTrading;
        std::vector<ItemStack> mItems;
    };

}

#endif
