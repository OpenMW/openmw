
#include "statemanagerimp.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"

MWState::StateManager::StateManager()
: mQuitRequest (false), mRunning (false)
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

bool MWState::StateManager::isGameRunning() const
{
    return mRunning;
}

void MWState::StateManager::newGame (bool bypass)
{
    if (mRunning)
    {
        MWBase::Environment::get().getDialogueManager()->clear();
        MWBase::Environment::get().getJournal()->clear();
        mRunning = false;
    }

    if (!bypass)
    {
        /// \todo extract cleanup code
        MWBase::Environment::get().getWorld()->startNewGame();
        MWBase::Environment::get().getWindowManager()->setNewGame (true);
    }

    mRunning = true;
}
