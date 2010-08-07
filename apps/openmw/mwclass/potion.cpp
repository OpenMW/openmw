
#include "potion.hpp"

#include <components/esm/loadalch.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Potion::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData> *ref =
            ptr.get<ESM::Potion>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Potion::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Potion::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.potions);
    }

    std::string Potion::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Potion, MWWorld::RefData> *ref =
            ptr.get<ESM::Potion>();

        return ref->base->script;
    }

    void Potion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Potion);

        registerClass (typeid (ESM::Potion).name(), instance);
    }
}
