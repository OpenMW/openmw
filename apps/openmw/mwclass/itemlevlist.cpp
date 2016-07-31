#include "itemlevlist.hpp"

#include <components/esm/loadlevlist.hpp>

namespace MWClass
{

    std::string ItemLevList::getName (const MWWorld::ConstPtr& ptr) const
    {
        return "";
    }

    void ItemLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ItemLevList);

        registerClass (typeid (ESM::ItemLevList).name(), instance);
    }
}
