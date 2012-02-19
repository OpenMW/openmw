
#include "weapon.hpp"

#include <components/esm/loadweap.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"


#include "containerutil.hpp"

namespace MWClass
{
    void Weapon::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData> *ref =
            ptr.get<ESM::Weapon>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        
        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(),true);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Weapon::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData> *ref =
            ptr.get<ESM::Weapon>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Weapon::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData> *ref =
            ptr.get<ESM::Weapon>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Weapon::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    bool Weapon::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        return true;
    }

    int Weapon::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData> *ref =
            ptr.get<ESM::Weapon>();

        return ref->base->data.health;
    }

    void Weapon::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.weapons);
    }

    std::string Weapon::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Weapon, MWWorld::RefData> *ref =
            ptr.get<ESM::Weapon>();

        return ref->base->script;
    }

    void Weapon::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Weapon);

        registerClass (typeid (ESM::Weapon).name(), instance);
    }
}
