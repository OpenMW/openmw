#include "itemlevlist.hpp"

#include <components/esm3/loadlevlist.hpp>

namespace MWClass
{
    ItemLevList::ItemLevList()
        : MWWorld::RegisteredClass<ItemLevList>(ESM::ItemLevList::sRecordId)
    {
    }

    std::string ItemLevList::getName (const MWWorld::ConstPtr& ptr) const
    {
        return "";
    }

    bool ItemLevList::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }
}
