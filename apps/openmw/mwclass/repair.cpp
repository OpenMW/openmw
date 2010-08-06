
#include "repair.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Repair::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData> *ref =
            ptr.get<ESM::Repair>();

        return ref->base->name;
    }

    void Repair::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.repairs);
    }

    std::string Repair::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData> *ref =
            ptr.get<ESM::Repair>();

        return ref->base->script;
    }

    void Repair::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Repair);

        registerClass (typeid (ESM::Repair).name(), instance);
    }
}
