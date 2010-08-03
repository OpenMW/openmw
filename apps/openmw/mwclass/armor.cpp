
#include "armor.hpp"

#include <components/esm/loadarmo.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

namespace MWClass
{
    bool Armor::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        return true;
    }

    int Armor::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->data.health;
    }

    void Armor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Armor);

        registerClass (typeid (ESM::Armor).name(), instance);
    }
}
