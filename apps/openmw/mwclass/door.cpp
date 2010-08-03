
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        return ref->base->name;
    }

    void Door::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Door);

        registerClass (typeid (ESM::Door).name(), instance);
    }
}
