
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

namespace MWWorld
{
    void Activator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Activator);

        registerClass (typeid (ESM::Activator).name(), instance);
    }
}
