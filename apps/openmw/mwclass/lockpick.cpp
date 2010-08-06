
#include "lockpick.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Lockpick::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->name;
    }

    void Lockpick::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.lockpicks);
    }

    std::string Lockpick::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->script;
    }

    void Lockpick::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Lockpick);

        registerClass (typeid (ESM::Tool).name(), instance);
    }
}
