
#include "repair.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Repair::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Repair, MWWorld::RefData> *ref =
            ptr.get<ESM::Repair>();

        return ref->base->name;
    }

    void Repair::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Repair);

        registerClass (typeid (ESM::Repair).name(), instance);
    }
}
