
#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

namespace MWClass
{
    void Clothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Clothing);

        registerClass (typeid (ESM::Clothing).name(), instance);
    }
}
