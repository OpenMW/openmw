
#include "creaturelevlist.hpp"

#include <components/esm/loadlevlist.hpp>

namespace MWClass
{
    void CreatureLevList::registerSelf()
    {
        boost::shared_ptr<Class> instance (new CreatureLevList);

        registerClass (typeid (ESM::CreatureLevList).name(), instance);
    }
}
