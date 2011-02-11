
#include "misc.hpp"

#include <components/esm/loadmisc.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "../mwrender/cellimp.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    void Misc::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Misc, MWWorld::RefData> *ref =
            ptr.get<ESM::Misc>();

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

    std::string Misc::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Misc, MWWorld::RefData> *ref =
            ptr.get<ESM::Misc>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Misc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Misc::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.miscItems);
    }

    std::string Misc::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Misc, MWWorld::RefData> *ref =
            ptr.get<ESM::Misc>();

        return ref->base->script;
    }

    void Misc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Misc);

        registerClass (typeid (ESM::Misc).name(), instance);
    }
}
