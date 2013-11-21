#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include "../mwbase/statemanager.hpp"

#include <boost/filesystem/path.hpp>

#include "charactermanager.hpp"

namespace MWState
{
    class StateManager : public MWBase::StateManager
    {
            bool mQuitRequest;
            State mState;
            CharacterManager mCharacterManager;

        public:

            StateManager (const boost::filesystem::path& saves);

            virtual void requestQuit();

            virtual bool hasQuitRequest() const;

            virtual State getState() const;

            virtual void newGame (bool bypass = false);
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.

            virtual void endGame();

            virtual void saveGame (const Slot *slot = 0);
            ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
            ///
            /// \note Slot must belong to the current character.

            virtual void loadGame (const Character *character, const Slot *slot);
            ///< Load a saved game file from \a slot.
            ///
            /// \note \a slot must belong to \a character.

            virtual Character *getCurrentCharacter();

            virtual CharacterIterator characterBegin();

            virtual CharacterIterator characterEnd();
    };
}

#endif
