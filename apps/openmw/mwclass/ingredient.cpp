
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

namespace MWClass
{
    void Ingredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Ingredient);

        registerClass (typeid (ESM::Ingredient).name(), instance);
    }
}
