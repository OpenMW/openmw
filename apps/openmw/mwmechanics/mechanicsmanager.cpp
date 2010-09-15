
#include "mechanicsmanager.hpp"

#include <components/esm_store/store.hpp>

#include "../mwgui/window_manager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

namespace MWMechanics
{
    MechanicsManager::MechanicsManager (MWWorld::Environment& environment)
    : mEnvironment (environment), mSetName (true)
    {

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

    }

    void MechanicsManager::setPlayerBirthsign (const std::string& id)
    {

    }

    void MechanicsManager::setPlayerClass (const std::string& id)
    {

    }

    void MechanicsManager::setPlayerClass (const ESM::Class& class_)
    {

    }
}
