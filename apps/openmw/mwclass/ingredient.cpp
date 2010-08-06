
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Ingredient::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->name;
    }

    void Ingredient::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.ingreds);
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->script;
    }

    void Ingredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Ingredient);

        registerClass (typeid (ESM::Ingredient).name(), instance);
    }
}
