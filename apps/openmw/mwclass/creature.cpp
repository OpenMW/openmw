
#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

namespace
{
    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWWorld::ContainerStore mContainerStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }
}

namespace MWClass
{
    void Creature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data (new CustomData);

            MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

            // creature stats
            data->mCreatureStats.getAttribute(0).set (ref->base->data.strength);
            data->mCreatureStats.getAttribute(1).set (ref->base->data.intelligence);
            data->mCreatureStats.getAttribute(2).set (ref->base->data.willpower);
            data->mCreatureStats.getAttribute(3).set (ref->base->data.agility);
            data->mCreatureStats.getAttribute(4).set (ref->base->data.speed);
            data->mCreatureStats.getAttribute(5).set (ref->base->data.endurance);
            data->mCreatureStats.getAttribute(6).set (ref->base->data.personality);
            data->mCreatureStats.getAttribute(7).set (ref->base->data.luck);
            data->mCreatureStats.getHealth().set (ref->base->data.health);
            data->mCreatureStats.getMagicka().set (ref->base->data.mana);
            data->mCreatureStats.getFatigue().set (ref->base->data.fatigue);

            data->mCreatureStats.setLevel(ref->base->data.level);

            data->mCreatureStats.setHello(ref->base->AI.hello);
            data->mCreatureStats.setFight(ref->base->AI.fight);
            data->mCreatureStats.setFlee(ref->base->AI.flee);
            data->mCreatureStats.setAlarm(ref->base->AI.alarm);

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Creature::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->mId;
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr);
    }

    void Creature::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()){
            physics.insertActorPhysics(ptr, model);
        }
        MWBase::Environment::get().getMechanicsManager()->addActor (ptr);
    }

    std::string Creature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        assert (ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";        
    }

    std::string Creature::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->name;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    boost::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionTalk (ptr));
    }

    MWWorld::ContainerStore& Creature::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    std::string Creature::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->base->script;
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }

    bool Creature::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for Creatures in combat mode.

        return true;
    }

    MWGui::ToolTipInfo Creature::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        info.text = text;

        return info;
    }

    float Creature::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Creature::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (8)).mMagnitude; // feather

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (7)).mMagnitude; // burden

        if (weight<0)
            weight = 0;

        return weight;
    }

    MWWorld::Ptr
    Creature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return MWWorld::Ptr(&cell.creatures.insert(*ref), &cell);
    }
}
