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
            bool mAskLoadRecent;
            State mState;
            CharacterManager mCharacterManager;
            double mTimePlayed;

        private:

            void cleanup();

        public:

            StateManager (const boost::filesystem::path& saves, const std::string& game);

            virtual void requestQuit();

            virtual bool hasQuitRequest() const;

            virtual void askLoadRecent();

            virtual State getState() const;

            virtual void newGame (bool bypass = false);
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.

            virtual void endGame();

            virtual void saveGame (const std::string& description, const Slot *slot = 0);
            ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
            ///
            /// \note Slot must belong to the current character.

            virtual void loadGame (const Character *character, const Slot *slot);
            ///< Load a saved game file from \a slot.
            ///
            /// \note \a slot must belong to \a character.

            virtual Character *getCurrentCharacter (bool create = true);
            ///< \param create Create a new character, if there is no current character.

            virtual CharacterIterator characterBegin();
            ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
            /// iterator.

            virtual CharacterIterator characterEnd();

            virtual void update (float duration);
    };
}

#endif
