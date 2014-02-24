#include "companionitemmodel.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"

namespace MWGui
{
    CompanionItemModel::CompanionItemModel(const MWWorld::Ptr &actor)
        : InventoryItemModel(actor)
    {
    }

    void CompanionItemModel::copyItem (const ItemStack& item, size_t count)
    {
        if (mActor.getClass().isNpc())
        {
            MWMechanics::NpcStats& stats = MWWorld::Class::get(mActor).getNpcStats(mActor);
            stats.modifyProfit(MWWorld::Class::get(item.mBase).getValue(item.mBase) * count);
        }

        InventoryItemModel::copyItem(item, count);
    }

    void CompanionItemModel::removeItem (const ItemStack& item, size_t count)
    {
        if (mActor.getClass().isNpc())
        {
            MWMechanics::NpcStats& stats = MWWorld::Class::get(mActor).getNpcStats(mActor);
            stats.modifyProfit(-MWWorld::Class::get(item.mBase).getValue(item.mBase) * count);
        }

        InventoryItemModel::removeItem(item, count);
    }
}
