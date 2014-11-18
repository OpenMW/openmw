
#include "itemlevlist.hpp"

#include <components/esm/loadlevlist.hpp>

namespace MWClass
{
    std::string ItemLevList::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::ItemLevList>()->mBase->mId;
    }

    std::string ItemLevList::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ItemLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ItemLevList);

        registerClass (typeid (ESM::ItemLevList).name(), instance);
    }
}
