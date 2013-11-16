#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include "../mwbase/statemanager.hpp"

namespace MWState
{
    class StateManager : public MWBase::StateManager
    {
            bool mQuitRequest;

        public:

            StateManager();

            virtual void requestQuit();

            virtual bool hasQuitRequest() const;
    };
}

#endif
