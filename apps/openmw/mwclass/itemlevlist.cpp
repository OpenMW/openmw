#include "itemlevlist.hpp"

#include <components/esm3/loadlevlist.hpp>

namespace MWClass
{

    std::string ItemLevList::getName (const MWWorld::ConstPtr& ptr) const
    {
        return "";
    }

    bool ItemLevList::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return false;
    }

    void ItemLevList::registerSelf()
    {
        std::shared_ptr<Class> instance (new ItemLevList);

        registerClass (ESM::ItemLevList::sRecordId, instance);
    }
}
