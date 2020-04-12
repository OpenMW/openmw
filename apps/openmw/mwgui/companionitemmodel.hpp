#ifndef MWGUI_COMPANION_ITEM_MODEL_H
#define MWGUI_COMPANION_ITEM_MODEL_H

#include "inventoryitemmodel.hpp"

namespace MWGui
{

    /// @brief The companion item model keeps track of the companion's profit by
    /// monitoring which items are being added to and removed from the model.
    class CompanionItemModel : public InventoryItemModel
    {
    public:
        CompanionItemModel (const MWWorld::Ptr& actor);

        virtual MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool allowAutoEquip = true);
        virtual void removeItem (const ItemStack& item, size_t count);

        bool hasProfit(const MWWorld::Ptr& actor);
    };

}

#endif
