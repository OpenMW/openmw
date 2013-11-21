#ifndef GAME_MWSTATE_STATEMANAGER_H
#define GAME_MWSTATE_STATEMANAGER_H

#include <vector>

namespace MWState
{
    struct Slot;
    class Character;
}

namespace MWBase
{
    /// \brief Interface for game state manager (implemented in MWState)
    class StateManager
    {
        public:

            enum State
            {
                State_NoGame,
                State_Ended,
                State_Running
            };

            typedef std::vector<MWState::Character>::const_iterator CharacterIterator;

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

            virtual State getState() const = 0;

            virtual void newGame (bool bypass = false) = 0;
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.

            virtual void endGame() = 0;

            virtual void saveGame (const MWState::Slot *slot = 0) = 0;
            ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
            ///
            /// \note Slot must belong to the current character.

            virtual void loadGame (const MWState::Character *character, const MWState::Slot *slot) = 0;
            ///< Load a saved game file from \a slot.
            ///
            /// \note \a slot must belong to \a character.

            virtual MWState::Character *getCurrentCharacter() = 0;
            ///< \attention Do not call this function to check if there is a current character.
            /// Instead, assume there is a character if getState() == Running.

            virtual CharacterIterator characterBegin() = 0;
            ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
            /// iterator.

            virtual CharacterIterator characterEnd() = 0;
    };
}

#endif
