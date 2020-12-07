#ifndef GAME_MWSTATE_STATEMANAGER_H
#define GAME_MWSTATE_STATEMANAGER_H

#include <list>
#include <string>

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

            typedef std::list<MWState::Character>::const_iterator CharacterIterator;

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

            virtual void askLoadRecent() = 0;

            virtual State getState() const = 0;

            virtual void newGame (bool bypass = false) = 0;
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.

            virtual void endGame() = 0;

            virtual void resumeGame() = 0;

            virtual void deleteGame (const MWState::Character *character, const MWState::Slot *slot) = 0;

            virtual void saveGame (const std::string& description, const MWState::Slot *slot = nullptr) = 0;
            ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
            ///
            /// \note Slot must belong to the current character.

            virtual void loadGame (const std::string& filepath) = 0;
            ///< Load a saved game directly from the given file path. This will search the CharacterManager
            /// for a Character containing this save file, and set this Character current if one was found.
            /// Otherwise, a new Character will be created.

            virtual void loadGame (const MWState::Character *character, const std::string& filepath) = 0;
            ///< Load a saved game file belonging to the given character.

            ///Simple saver, writes over the file if already existing
            /** Used for quick save and autosave **/
            virtual void quickSave(std::string = "Quicksave")=0;

            ///Simple loader, loads the last saved file
            /** Used for quickload **/
            virtual void quickLoad()=0;

            virtual MWState::Character *getCurrentCharacter () = 0;
            ///< @note May return null.

            virtual CharacterIterator characterBegin() = 0;
            ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
            /// iterator.

            virtual CharacterIterator characterEnd() = 0;

            virtual void update (float duration) = 0;
    };
}

#endif
