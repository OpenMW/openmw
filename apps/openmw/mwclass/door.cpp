
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

namespace MWClass
{
    void Door::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Door);

        registerClass (typeid (ESM::Door).name(), instance);
    }
}
