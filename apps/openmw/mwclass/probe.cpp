
#include "probe.hpp"

#include <components/esm/loadlocks.hpp>

namespace MWClass
{
    void Probe::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Probe);

        registerClass (typeid (ESM::Probe).name(), instance);
    }
}
