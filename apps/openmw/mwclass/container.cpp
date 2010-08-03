
#include "container.hpp"

#include <components/esm/loadcont.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Container::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData> *ref =
            ptr.get<ESM::Container>();

        return ref->base->name;
    }

    void Container::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Container);

        registerClass (typeid (ESM::Container).name(), instance);
    }
}
