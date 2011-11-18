
#include "probe.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "containerutil.hpp"
#include "../mwrender/objects.hpp"

namespace MWClass
{
    void Probe::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData> *ref =
            ptr.get<ESM::Probe>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        
        if (!model.empty())
        {
            MWRender::Objects objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Probe::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData> *ref =
            ptr.get<ESM::Probe>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }


    std::string Probe::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Probe, MWWorld::RefData> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->name;
    }
    boost::shared_ptr<MWWorld::Action> Probe::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
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
