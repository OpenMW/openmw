#ifndef GAME_MWSTATE_STATEMANAGER_H
#define GAME_MWSTATE_STATEMANAGER_H

namespace MWBase
{
    /// \brief Interface for game state manager (implemented in MWState)
    class StateManager
    {
        private:

            StateManager (const StateManager&);
            ///< not implemented

            StateManager& operator= (const StateManager&);
            ///< not implemented

        public:

            StateManager() {}

            virtual ~StateManager() {}

            virtual void requestQuit() = 0;

            virtual bool hasQuitRequest() const = 0;

            virtual bool isGameRunning() const = 0;

            virtual void newGame (bool bypass = false) = 0;
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.
    };
}

#endif
