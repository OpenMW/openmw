
#include "container.hpp"

#include <components/esm/loadcont.hpp>

namespace MWClass
{
    void Container::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Container);

        registerClass (typeid (ESM::Container).name(), instance);
    }
}
