
#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/cellimp.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

namespace MWClass
{
    std::string Creature::getId (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->mId;
    }

    void Creature::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        if (!model.empty())
        {
            MWRender::Rendering rendering (cellRender, ref->ref);
            cellRender.insertMesh("meshes\\" + model);
            cellRender.insertActorPhysics();
            ref->mData.setHandle (rendering.end (ref->mData.isEnabled()));
        }
    }

    void Creature::enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        environment.mMechanicsManager->addActor (ptr);
    }

    void Creature::disable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        environment.mMechanicsManager->removeActor (ptr);
    }

    std::string Creature::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->name;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCreatureStats().get())
        {
            boost::shared_ptr<MWMechanics::CreatureStats> stats (
                new MWMechanics::CreatureStats);

            ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref = ptr.get<ESM::Creature>();

            stats->mAttributes[0].set (ref->base->data.strength);
            stats->mAttributes[1].set (ref->base->data.intelligence);
            stats->mAttributes[2].set (ref->base->data.willpower);
            stats->mAttributes[3].set (ref->base->data.agility);
            stats->mAttributes[4].set (ref->base->data.speed);
            stats->mAttributes[5].set (ref->base->data.endurance);
            stats->mAttributes[6].set (ref->base->data.personality);
            stats->mAttributes[7].set (ref->base->data.luck);
            stats->mDynamic[0].set (ref->base->data.health);
            stats->mDynamic[1].set (ref->base->data.mana);
            stats->mDynamic[2].set (ref->base->data.fatigue);

            stats->mLevel = ref->base->data.level;

            ptr.getRefData().getCreatureStats() = stats;
        }

        return *ptr.getRefData().getCreatureStats();
    }

    boost::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionTalk (ptr));
    }

    MWWorld::ContainerStore<MWWorld::RefData>& Creature::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        if (!ptr.getRefData().getContainerStore().get())
        {
            boost::shared_ptr<MWWorld::ContainerStore<MWWorld::RefData> > store (
                new MWWorld::ContainerStore<MWWorld::RefData>);

            // TODO add initial content

            ptr.getRefData().getContainerStore() = store;
        }

        return *ptr.getRefData().getContainerStore();
    }

    std::string Creature::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Creature, MWWorld::RefData> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->script;
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }
}
