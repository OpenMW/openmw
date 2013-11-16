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
    };
}

#endif
