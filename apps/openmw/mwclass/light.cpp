
#include "light.hpp"

#include <components/esm/loadligh.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/nullaction.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    std::string Light::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (ref->base->model.empty())
            return "";

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (!(ref->base->data.flags & ESM::Light::Carry))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Light::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.lights);
    }

    std::string Light::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        return ref->base->script;
    }

    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }
}
