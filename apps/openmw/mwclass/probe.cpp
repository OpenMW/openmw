
#include "probe.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Probe::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->name;
    }

    void Probe::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.probes);
    }

    std::string Probe::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->script;
    }

    void Probe::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Probe);

        registerClass (typeid (ESM::Probe).name(), instance);
    }
}
