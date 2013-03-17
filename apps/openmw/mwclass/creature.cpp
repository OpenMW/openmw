
#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/actors.hpp"

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
            data->mCreatureStats.getAttribute(0).set (ref->mBase->mData.mStrength);
            data->mCreatureStats.getAttribute(1).set (ref->mBase->mData.mIntelligence);
            data->mCreatureStats.getAttribute(2).set (ref->mBase->mData.mWillpower);
            data->mCreatureStats.getAttribute(3).set (ref->mBase->mData.mAgility);
            data->mCreatureStats.getAttribute(4).set (ref->mBase->mData.mSpeed);
            data->mCreatureStats.getAttribute(5).set (ref->mBase->mData.mEndurance);
            data->mCreatureStats.getAttribute(6).set (ref->mBase->mData.mPersonality);
            data->mCreatureStats.getAttribute(7).set (ref->mBase->mData.mLuck);
            data->mCreatureStats.setHealth (ref->mBase->mData.mHealth);
            data->mCreatureStats.setMagicka (ref->mBase->mData.mMana);
            data->mCreatureStats.setFatigue (ref->mBase->mData.mFatigue);

            data->mCreatureStats.setLevel(ref->mBase->mData.mLevel);

            data->mCreatureStats.setAiSetting (0, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (1, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (2, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (3, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
                data->mCreatureStats.getSpells().add (*iter);

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Creature::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mId;
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr);
    }

    void Creature::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addActor(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Creature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Creature::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    boost::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (MWWorld::Class::get (ptr).getCreatureStats (ptr).isDead())
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionOpen(ptr, true));
        else
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

        return ref->mBase->mScript;
    }

    bool Creature::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Essential;
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
        info.caption = ref->mBase->mName;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }

    float Creature::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        /// \todo add Shield magic effect magnitude here, controlled by a GMST (Vanilla vs MCP behaviour)
        return 0.f;
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

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Feather)).mMagnitude;

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Burden)).mMagnitude;

        if (weight<0)
            weight = 0;

        return weight;
    }

    MWWorld::Ptr
    Creature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return MWWorld::Ptr(&cell.mCreatures.insert(*ref), &cell);
    }
}
