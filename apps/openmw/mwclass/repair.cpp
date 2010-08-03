
#include "repair.hpp"

#include <components/esm/loadlocks.hpp>

namespace MWClass
{
    void Repair::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Repair);

        registerClass (typeid (ESM::Repair).name(), instance);
    }
}
