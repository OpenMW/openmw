
#include "statemanagerimp.hpp"

MWState::StateManager::StateManager()
: mQuitRequest (false)
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