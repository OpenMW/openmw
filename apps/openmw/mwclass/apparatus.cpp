
#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

namespace MWClass
{
    void Apparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Apparatus);

        registerClass (typeid (ESM::Apparatus).name(), instance);
    }
}
