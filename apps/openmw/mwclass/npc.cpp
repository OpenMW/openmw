
#include "npc.hpp"

#include <memory>

#include <boost/algorithm/string.hpp>

#include <OgreSceneNode.h>

#include <components/esm/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    const Ogre::Radian kOgrePi (Ogre::Math::PI);
    const Ogre::Radian kOgrePiOverTwo (Ogre::Math::PI / Ogre::Real(2.0));

    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::CreatureStats mCreatureStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }
}

namespace MWClass
{
    void Npc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data (new CustomData);

            MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

            // NPC stats
            if (!ref->base->mFaction.empty())
            {
                std::string faction = ref->base->mFaction;
                boost::algorithm::to_lower(faction);
                if(ref->base->mNpdt52.mGold != -10)
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->base->mNpdt52.mRank;
                }
                else
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->base->mNpdt12.mRank;
                }
            }

            // creature stats
            if(ref->base->mNpdt52.mGold != -10)
            {
                for (int i=0; i<27; ++i)
                    data->mNpcStats.getSkill (i).setBase (ref->base->mNpdt52.mSkills[i]);

                data->mCreatureStats.getAttribute(0).set (ref->base->mNpdt52.mStrength);
                data->mCreatureStats.getAttribute(1).set (ref->base->mNpdt52.mIntelligence);
                data->mCreatureStats.getAttribute(2).set (ref->base->mNpdt52.mWillpower);
                data->mCreatureStats.getAttribute(3).set (ref->base->mNpdt52.mAgility);
                data->mCreatureStats.getAttribute(4).set (ref->base->mNpdt52.mSpeed);
                data->mCreatureStats.getAttribute(5).set (ref->base->mNpdt52.mEndurance);
                data->mCreatureStats.getAttribute(6).set (ref->base->mNpdt52.mPersonality);
                data->mCreatureStats.getAttribute(7).set (ref->base->mNpdt52.mLuck);
                data->mCreatureStats.getHealth().set (ref->base->mNpdt52.mHealth);
                data->mCreatureStats.getMagicka().set (ref->base->mNpdt52.mMana);
                data->mCreatureStats.getFatigue().set (ref->base->mNpdt52.mFatigue);

                data->mCreatureStats.setLevel(ref->base->mNpdt52.mLevel);
            }
            else
            {
                /// \todo do something with mNpdt12 maybe:p
            }

            data->mCreatureStats.setHello(ref->base->mAiData.mHello);
            data->mCreatureStats.setFight(ref->base->mAiData.mFight);
            data->mCreatureStats.setFlee(ref->base->mAiData.mFlee);
            data->mCreatureStats.setAlarm(ref->base->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->base->mSpells.mList.begin());
                iter!=ref->base->mSpells.mList.end(); ++iter)
                data->mCreatureStats.getSpells().add (*iter);

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Npc::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->mId;
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getActors().insertNPC(ptr, getInventoryStore(ptr));
    }

    void Npc::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        physics.insertActorPhysics(ptr, getModel(ptr));
        MWBase::Environment::get().getMechanicsManager()->addActor(ptr);
    }

    std::string Npc::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();
        assert(ref->base != NULL);

        std::string headID = ref->base->mHead;

        int end = headID.find_last_of("head_") - 4;
        std::string bodyRaceID = headID.substr(0, end);

        std::string model = "meshes\\base_anim.nif";
        if (bodyRaceID == "b_n_khajiit_m_" ||
            bodyRaceID == "b_n_khajiit_f_" ||
            bodyRaceID == "b_n_argonian_m_" ||
            bodyRaceID == "b_n_argonian_f_")
        {
            model = "meshes\\base_animkna.nif";
        }
        return model;

    }

    std::string Npc::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->mName;
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    boost::shared_ptr<MWWorld::Action> Npc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionTalk (ptr));
    }

    MWWorld::ContainerStore& Npc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    std::string Npc::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->mScript;
    }

    void Npc::setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceRun, force);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak, force);
                break;

            case Combat:

                throw std::runtime_error ("combat stance not enforcable for NPCs");
        }
    }

    void Npc::setStance (const MWWorld::Ptr& ptr, Stance stance, bool set) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Run, set);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Sneak, set);
                break;

            case Combat:

                // Combat stance ignored for now; need to be determined based on draw state instead of
                // being maunally set.
                break;
        }
    }

    bool Npc::getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceRun))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Run);

            case Sneak:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Sneak);

            case Combat:

                return false;
        }

        return false;
    }

    float Npc::getSpeed (const MWWorld::Ptr& ptr) const
    {
        return getStance (ptr, Run) ? 600 : 300; // TODO calculate these values from stats
    }

    MWMechanics::Movement& Npc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 Npc::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        Ogre::Vector3 vector (0, 0, 0);

        vector.x = getMovementSettings (ptr).mLeftRight * 127;
        vector.y = getMovementSettings (ptr).mForwardBackward * 127;
        vector.z = getMovementSettings(ptr).mUpDown * 127;

        //if (getStance (ptr, Run, false))
        //    vector *= 2;

        return vector;
    }

    void Npc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Npc);
        registerClass (typeid (ESM::NPC).name(), instance);
    }

    bool Npc::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for NPCs in combat mode.

        return true;
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->mName;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->mScript, "Script");
        info.text = text;

        return info;
    }

    float Npc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Npc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (8)).mMagnitude; // feather

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (7)).mMagnitude; // burden

        if (weight<0)
            weight = 0;

        return weight;
    }

    bool Npc::apply (const MWWorld::Ptr& ptr, const std::string& id,
        const MWWorld::Ptr& actor) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        /// \todo consider instant effects

        return stats.getActiveSpells().addSpell (id, actor);
    }

    void Npc::skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        const ESM::Class *class_ = MWBase::Environment::get().getWorld()->getStore().classes.find (
            ref->base->mClass);

        stats.useSkill (skill, *class_, usageType);
    }

    void Npc::adjustRotation(const MWWorld::Ptr& ptr,float& x,float& y,float& z) const
    {
        y = 0;
        x = 0;
    }

    MWWorld::Ptr
    Npc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return MWWorld::Ptr(&cell.npcs.insert(*ref), &cell);
    }
}
