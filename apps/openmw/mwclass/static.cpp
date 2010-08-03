
#include "static.hpp"

#include <components/esm/loadstat.hpp>

namespace MWClass
{
    void Static::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Static);

        registerClass (typeid (ESM::Static).name(), instance);
    }
}
