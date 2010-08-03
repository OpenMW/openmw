
#include "lockpick.hpp"

#include <components/esm/loadlocks.hpp>

namespace MWClass
{
    void Lockpick::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Lockpick);

        registerClass (typeid (ESM::Tool).name(), instance);
    }
}
