
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Activator::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        return ref->base->name;
    }

    std::string Activator::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        return ref->base->script;
    }

    void Activator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Activator);

        registerClass (typeid (ESM::Activator).name(), instance);
    }
}
