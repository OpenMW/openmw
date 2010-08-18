
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwrender/cellimp.hpp"

namespace MWClass
{
    void Activator::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        if (!model.empty())
        {
            cellRender.insertBegin (ref->ref);
            cellRender.insertMesh ("meshes\\" + model);
            ref->mData.setHandle (cellRender.insertEnd (ref->mData.isEnabled()));
        }
    }

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
