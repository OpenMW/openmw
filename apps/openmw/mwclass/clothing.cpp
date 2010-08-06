
#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Clothing::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->name;
    }

    void Clothing::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.clothes);
    }

    std::string Clothing::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->script;
    }

    void Clothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Clothing);

        registerClass (typeid (ESM::Clothing).name(), instance);
    }
}
