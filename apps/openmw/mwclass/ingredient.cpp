
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Ingredient::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Ingredient::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
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
        environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr, environment), 1.0, 1.0, false, true);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->script;
    }

    int Ingredient::getValue (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Ingredient, MWWorld::RefData> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->data.value;
    }

    void Ingredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Ingredient);

        registerClass (typeid (ESM::Ingredient).name(), instance);
    }

    std::string Ingredient::getUpSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        return std::string("Item Ingredient Up");
    }

    std::string Ingredient::getDownSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        return std::string("Item Ingredient Down");
    }
}
