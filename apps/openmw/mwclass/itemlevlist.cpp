#include "itemlevlist.hpp"

#include <components/esm/loadlevlist.hpp>

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

        registerClass (typeid (ESM::ItemLevList).name(), instance);
    }
}
