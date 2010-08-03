
#include "light.hpp"

#include <components/esm/loadligh.hpp>

namespace MWClass
{
    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }
}
