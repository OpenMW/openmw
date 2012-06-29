
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

#include "../mwgui/window_manager.hpp"

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
            data->mCreatureStats.mAttributes[0].set (ref->base->data.strength);
            data->mCreatureStats.mAttributes[1].set (ref->base->data.intelligence);
            data->mCreatureStats.mAttributes[2].set (ref->base->data.willpower);
            data->mCreatureStats.mAttributes[3].set (ref->base->data.agility);
            data->mCreatureStats.mAttributes[4].set (ref->base->data.speed);
            data->mCreatureStats.mAttributes[5].set (ref->base->data.endurance);
            data->mCreatureStats.mAttributes[6].set (ref->base->data.personality);
            data->mCreatureStats.mAttributes[7].set (ref->base->data.luck);
            data->mCreatureStats.mDynamic[0].set (ref->base->data.health);
            data->mCreatureStats.mDynamic[1].set (ref->base->data.mana);
            data->mCreatureStats.mDynamic[2].set (ref->base->data.fatigue);

            data->mCreatureStats.mLevel = ref->base->data.level;

            data->mCreatureStats.mHello = ref->base->AI.hello;
            data->mCreatureStats.mFight = ref->base->AI.fight;
            data->mCreatureStats.mFlee = ref->base->AI.flee;
            data->mCreatureStats.mAlarm = ref->base->AI.alarm;

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
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertActorPhysics(ptr, "meshes\\" + model);
        }

        MWBase::Environment::get().getMechanicsManager()->addActor (ptr);
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
        return stats.mAttributes[0].getModified()*5;
    }

    float Creature::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.mMagicEffects.get (MWMechanics::EffectKey (8)).mMagnitude; // feather

        weight += stats.mMagicEffects.get (MWMechanics::EffectKey (7)).mMagnitude; // burden

        if (weight<0)
            weight = 0;

        return weight;
    }
}
