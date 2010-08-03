
#include "potion.hpp"

#include <components/esm/loadalch.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    std::string Potion::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData> *ref =
            ptr.get<ESM::Potion>();

        return ref->base->name;
    }

    void Potion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Potion);

        registerClass (typeid (ESM::Potion).name(), instance);
    }
}
