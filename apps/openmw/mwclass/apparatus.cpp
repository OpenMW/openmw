
#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "../mwrender/cellimp.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    void Apparatus::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        if (!model.empty())
        {
            MWRender::Rendering rendering (cellRender, ref->ref);
            cellRender.insertMesh ("meshes\\" + model);
            cellRender.insertObjectPhysics();
            ref->mData.setHandle (rendering.end (ref->mData.isEnabled()));
        }
    }

    std::string Apparatus::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Apparatus::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Apparatus::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.appas);
    }

    std::string Apparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->script;
    }

    void Apparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Apparatus);

        registerClass (typeid (ESM::Apparatus).name(), instance);
    }
}
