
#include "mechanicsmanager.hpp"

#include <components/esm_store/store.hpp>

#include "../mwgui/window_manager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

namespace MWMechanics
{
    void MechanicsManager::buildPlayer()
    {
        MWWorld::Ptr ptr = mEnvironment.mWorld->getPlayerPos().getPlayer();

        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (ptr).getCreatureStats (ptr);
//        MWMechanics::NpcStats& npcStats = MWWorld::Class::get (ptr).getNpcStats (ptr);

        const ESM::NPC *player = ptr.get<ESM::NPC>()->base;

        // reset
        creatureStats.mAttributes[0].setBase (player->npdt52.strength);
        creatureStats.mAttributes[1].setBase (player->npdt52.intelligence);
        creatureStats.mAttributes[2].setBase (player->npdt52.willpower);
        creatureStats.mAttributes[3].setBase (player->npdt52.agility);
        creatureStats.mAttributes[4].setBase (player->npdt52.speed);
        creatureStats.mAttributes[5].setBase (player->npdt52.endurance);
        creatureStats.mAttributes[6].setBase (player->npdt52.personality);
        creatureStats.mAttributes[7].setBase (player->npdt52.luck);


        // race

        // birthsign

        // class

        // calculate dynamic stats

    }

    MechanicsManager::MechanicsManager (MWWorld::Environment& environment)
    : mEnvironment (environment), mSetName (true)
    {
        buildPlayer();
    }

    void MechanicsManager::addActor (const MWWorld::Ptr& ptr)
    {
        mActors.insert (ptr);
    }

    void MechanicsManager::removeActor (const MWWorld::Ptr& ptr)
    {
        mActors.erase (ptr);
    }

    void MechanicsManager::dropActors (const MWWorld::Ptr::CellStore *cellStore)
    {
        std::set<MWWorld::Ptr>::iterator iter = mActors.begin();

        while (iter!=mActors.end())
            if (iter->getCell()==cellStore)
            {
                mActors.erase (iter++);
            }
            else
                ++iter;
    }

    void MechanicsManager::watchActor (const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void MechanicsManager::update()
    {
        if (!mWatched.isEmpty())
        {
            MWMechanics::CreatureStats& stats =
                MWWorld::Class::get (mWatched).getCreatureStats (mWatched);

            static const char *attributeNames[8] =
            {
                "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
                "AttribVal6", "AttribVal7", "AttribVal8"
            };

            static const char *dynamicNames[3] =
            {
                "HBar", "MBar", "FBar"
            };

            for (int i=0; i<8; ++i)
            {
                if (stats.mAttributes[i]!=mWatchedCreature.mAttributes[i])
                {
                    mWatchedCreature.mAttributes[i] = stats.mAttributes[i];

                    mEnvironment.mWindowManager->setValue (attributeNames[i], stats.mAttributes[i]);
                }
            }

            for (int i=0; i<3; ++i)
            {
                if (stats.mDynamic[i]!=mWatchedCreature.mDynamic[i])
                {
                    mWatchedCreature.mDynamic[i] = stats.mDynamic[i];

                    mEnvironment.mWindowManager->setValue (dynamicNames[i], stats.mDynamic[i]);
                }
            }
        }

        if (mSetName)
        {
            mEnvironment.mWindowManager->setValue ("name", mEnvironment.mWorld->getPlayerPos().getName());
            mSetName = false;
        }
    }

    void MechanicsManager::setPlayerName (const std::string& name)
    {
        mEnvironment.mWorld->getPlayerPos().setName (name);
        mSetName = true;
    }

    void MechanicsManager::setPlayerRace (const std::string& race, bool male)
    {


        buildPlayer();
    }

    void MechanicsManager::setPlayerBirthsign (const std::string& id)
    {


        buildPlayer();
    }

    void MechanicsManager::setPlayerClass (const std::string& id)
    {

        buildPlayer();
    }

    void MechanicsManager::setPlayerClass (const ESM::Class& class_)
    {

        buildPlayer();
    }
}
