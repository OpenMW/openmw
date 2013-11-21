
#include "statemanagerimp.hpp"

#include <components/esm/esmwriter.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

MWState::StateManager::StateManager (const boost::filesystem::path& saves)
: mQuitRequest (false), mState (State_NoGame), mCharacterManager (saves)
{

}

void MWState::StateManager::requestQuit()
{
    mQuitRequest = true;
}

bool MWState::StateManager::hasQuitRequest() const
{
    return mQuitRequest;
}

MWState::StateManager::State MWState::StateManager::getState() const
{
    return mState;
}

void MWState::StateManager::newGame (bool bypass)
{
    if (mState!=State_NoGame)
    {
        MWBase::Environment::get().getDialogueManager()->clear();
        MWBase::Environment::get().getJournal()->clear();
        mState = State_NoGame;
        mCharacterManager.clearCurrentCharacter();
    }

    if (!bypass)
    {
        /// \todo extract cleanup code
        MWBase::Environment::get().getWorld()->startNewGame();
        MWBase::Environment::get().getWindowManager()->setNewGame (true);
    }

    mState = State_Running;
}

void MWState::StateManager::endGame()
{
    mState = State_Ended;
}

void MWState::StateManager::saveGame (const Slot *slot)
{
    ESM::SavedGame profile;

    MWBase::World& world = *MWBase::Environment::get().getWorld();

    MWWorld::Ptr player = world.getPlayer().getPlayer();

    /// \todo store content file list
    profile.mPlayerName = player.getClass().getName (player);
    profile.mPlayerLevel = player.getClass().getNpcStats (player).getLevel();
    profile.mPlayerClass = player.get<ESM::NPC>()->mBase->mId;
    /// \todo player cell
    /// \todo gamehour
    profile.mInGameTime.mDay = world.getDay();
    profile.mInGameTime.mMonth = world.getMonth();
    /// \todo year
    /// \todo time played

    if (!slot)
        slot = mCharacterManager.getCurrentCharacter()->createSlot (profile);
    else
        slot = mCharacterManager.getCurrentCharacter()->updateSlot (slot, profile);

    std::ofstream stream (slot->mPath.string().c_str());
    ESM::ESMWriter writer;
//    writer.setFormat ();
    writer.save (stream);
    writer.startRecord ("SAVE");
    slot->mProfile.save (writer);
    writer.endRecord ("SAVE");
    writer.close();
}

MWState::Character *MWState::StateManager::getCurrentCharacter()
{
    return mCharacterManager.getCurrentCharacter();
}