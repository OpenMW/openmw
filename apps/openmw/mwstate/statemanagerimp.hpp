#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include "../mwbase/statemanager.hpp"

namespace MWState
{
    class StateManager : public MWBase::StateManager
    {
            bool mQuitRequest;
            bool mRunning;

        public:

            StateManager();

            virtual void requestQuit();

            virtual bool hasQuitRequest() const;

            virtual bool isGameRunning() const;

            virtual void newGame (bool bypass = false);
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.
    };
}

#endif
