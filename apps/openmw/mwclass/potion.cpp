
#include "potion.hpp"

#include <components/esm/loadalch.hpp>

namespace MWClass
{
    void Potion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Potion);

        registerClass (typeid (ESM::Potion).name(), instance);
    }
}
