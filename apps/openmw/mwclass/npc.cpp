
#include "npc.hpp"

#include <memory>

#include <OgreSceneNode.h>

#include <components/esm/loadnpc.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"

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

            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref = ptr.get<ESM::NPC>();

            // NPC stats
            if (!ref->base->faction.empty())
            {
                // TODO research how initial rank is stored. The information in loadnpc.hpp are at
                // best very unclear.
                data->mNpcStats.mFactionRank[ref->base->faction] = 0;
            }

            for (int i=0; i<27; ++i)
                data->mNpcStats.mSkill[i].setBase (ref->base->npdt52.skills[i]);

            // creature stats
            data->mCreatureStats.mAttributes[0].set (ref->base->npdt52.strength);
            data->mCreatureStats.mAttributes[1].set (ref->base->npdt52.intelligence);
            data->mCreatureStats.mAttributes[2].set (ref->base->npdt52.willpower);
            data->mCreatureStats.mAttributes[3].set (ref->base->npdt52.agility);
            data->mCreatureStats.mAttributes[4].set (ref->base->npdt52.speed);
            data->mCreatureStats.mAttributes[5].set (ref->base->npdt52.endurance);
            data->mCreatureStats.mAttributes[6].set (ref->base->npdt52.personality);
            data->mCreatureStats.mAttributes[7].set (ref->base->npdt52.luck);
            data->mCreatureStats.mDynamic[0].set (ref->base->npdt52.health);
            data->mCreatureStats.mDynamic[1].set (ref->base->npdt52.mana);
            data->mCreatureStats.mDynamic[2].set (ref->base->npdt52.fatigue);

            data->mCreatureStats.mLevel = ref->base->npdt52.level;

            // \todo add initial container content

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Npc::getId (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->mId;
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
		std::cout << "Inserting NPC\n";
        renderingInterface.getActors().insertNPC(ptr);
		
    }

    void Npc::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {


        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();


        assert (ref->base != NULL);
		 std::string headID = ref->base->head;
		 std::string bodyRaceID = headID.substr(0, headID.find_last_of("head_") - 4);
		 bool beast = bodyRaceID == "b_n_khajiit_m_" || bodyRaceID == "b_n_khajiit_f_" || bodyRaceID == "b_n_argonian_m_" || bodyRaceID == "b_n_argonian_f_";


        std::string smodel = "meshes\\base_anim.nif";
		if(beast)
			smodel = "meshes\\base_animkna.nif";
		physics.insertActorPhysics(ptr, smodel);


    }

    void Npc::enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        environment.mMechanicsManager->addActor (ptr);
    }

    void Npc::disable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        environment.mMechanicsManager->removeActor (ptr);
    }

    std::string Npc::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->name;
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
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
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
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *ref =
            ptr.get<ESM::NPC>();

        return ref->base->script;
    }

    void Npc::setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.mForceRun = force;
                break;

            case Sneak:

                stats.mForceSneak = force;
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

                stats.mRun = set;
                break;

            case Sneak:

                stats.mSneak = set;
                break;

            case Combat:

                stats.mCombat = set;
                break;
        }
    }

    bool Npc::getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                if (!ignoreForce && stats.mForceRun)
                    return true;

                return stats.mRun;

            case Sneak:

                if (!ignoreForce && stats.mForceSneak)
                    return true;

                return stats.mSneak;

            case Combat:

                return stats.mCombat;
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

        vector.x = - getMovementSettings (ptr).mLeftRight * 200;
        vector.y = getMovementSettings (ptr).mForwardBackward * 200;

        if (getStance (ptr, Run, false))
            vector *= 2;

        return vector;
    }

    void Npc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Npc);

        registerClass (typeid (ESM::NPC).name(), instance);
    }
}
