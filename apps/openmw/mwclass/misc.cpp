
#include "misc.hpp"

#include <components/esm/loadmisc.hpp>

namespace MWClass
{
    void Misc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Misc);

        registerClass (typeid (ESM::Misc).name(), instance);
    }
}
