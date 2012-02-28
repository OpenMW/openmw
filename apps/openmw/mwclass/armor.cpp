
#include "armor.hpp"

#include <components/esm/loadarmo.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"

#include "../mwrender/objects.hpp"

namespace MWClass
{
    void Armor::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Armor::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Armor::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Armor::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

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

    std::string Armor::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->script;
    }

    void Armor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Armor);

        registerClass (typeid (ESM::Armor).name(), instance);
    }
}
