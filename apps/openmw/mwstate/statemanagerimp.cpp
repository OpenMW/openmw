
#include "statemanagerimp.hpp"

#include <components/esm/esmwriter.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"

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
    }

    if (!bypass)
    {
        /// \todo extract cleanup code
        MWBase::Environment::get().getWorld()->startNewGame();
        MWBase::Environment::get().getWindowManager()->setNewGame (true);
    }

    mCharacterManager.createCharacter();

    mState = State_Running;
}

void MWState::StateManager::endGame()
{
    mState = State_Ended;
}

void MWState::StateManager::saveGame (const Slot *slot)
{
    ESM::SavedGame profile;

    /// \todo configure profile

    if (!slot)
        slot = mCharacterManager.getCurrentCharacter()->createSlot (profile);
    else
        slot = mCharacterManager.getCurrentCharacter()->updateSlot (slot, profile);

    ESM::ESMWriter writer;
//    writer.setFormat ();
    writer.save (slot->mPath.string());
    slot->mProfile.save (writer);
    writer.close();
}

MWState::Character *MWState::StateManager::getCurrentCharacter()
{
    return mCharacterManager.getCurrentCharacter();
}