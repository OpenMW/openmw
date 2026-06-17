#include "companionitemmodel.hpp"

#include "../mwworld/class.hpp"

namespace
{

    void modifyProfit(const MWWorld::Ptr& actor, int diff)
    {
        const ESM::RefId& script = actor.getClass().getScript(actor);
        if (!script.empty())
        {
            int profit = actor.getRefData().getLocals().getIntVar(script, "minimumprofit");
            profit += diff;
            actor.getRefData().getLocals().setVarByInt(script, "minimumprofit", profit);
        }
    }

}

namespace MWGui
{
    CompanionItemModel::CompanionItemModel(const MWWorld::Ptr& actor)
        : InventoryItemModel(actor)
    {
    }

    MWWorld::Ptr CompanionItemModel::addItem(const ItemStack& item, size_t count, bool allowAutoEquip)
    {
        if (hasProfit(mActor))
            modifyProfit(mActor, static_cast<int>(item.mBase.getClass().getValue(item.mBase) * count));

        return InventoryItemModel::addItem(item, count, allowAutoEquip);
    }

    MWWorld::Ptr CompanionItemModel::copyItem(const ItemStack& item, size_t count, bool allowAutoEquip)
    {
        if (hasProfit(mActor))
            modifyProfit(mActor, static_cast<int>(item.mBase.getClass().getValue(item.mBase) * count));

        return InventoryItemModel::copyItem(item, count, allowAutoEquip);
    }

    void CompanionItemModel::removeItem(const ItemStack& item, size_t count)
    {
        if (hasProfit(mActor))
            modifyProfit(mActor, -static_cast<int>(item.mBase.getClass().getValue(item.mBase) * count));

        InventoryItemModel::removeItem(item, count);
    }

    bool CompanionItemModel::hasProfit(const MWWorld::Ptr& actor)
    {
        const ESM::RefId& script = actor.getClass().getScript(actor);
        if (script.empty())
            return false;
        return actor.getRefData().getLocals().hasVar(script, "minimumprofit");
    }
}
