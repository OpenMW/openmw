
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "../mwrender/cellimp.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    void Ingredient::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

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

    std::string Ingredient::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Ingredient::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Ingredient::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.ingreds);
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->script;
    }

    void Ingredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Ingredient);

        registerClass (typeid (ESM::Ingredient).name(), instance);
    }
}
