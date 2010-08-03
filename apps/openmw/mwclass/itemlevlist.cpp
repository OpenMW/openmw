
#include "itemlevlist.hpp"

#include <components/esm/loadlevlist.hpp>

namespace MWClass
{
    void ItemLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ItemLevList);

        registerClass (typeid (ESM::ItemLevList).name(), instance);
    }
}
