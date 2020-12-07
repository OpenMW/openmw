#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include <map>

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

            void cleanup (bool force = false);

            bool verifyProfile (const ESM::SavedGame& profile) const;

            void writeScreenshot (std::vector<char>& imageData) const;

            std::map<int, int> buildContentFileIndexMap (const ESM::ESMReader& reader) const;

        public:

            StateManager (const boost::filesystem::path& saves, const std::string& game);

            void requestQuit() override;

            bool hasQuitRequest() const override;

            void askLoadRecent() override;

            State getState() const override;

            void newGame (bool bypass = false) override;
            ///< Start a new game.
            ///
            /// \param bypass Skip new game mechanics.

            void endGame() override;

            void resumeGame() override;

            void deleteGame (const MWState::Character *character, const MWState::Slot *slot) override;
            ///< Delete a saved game slot from this character. If all save slots are deleted, the character will be deleted too.

            void saveGame (const std::string& description, const Slot *slot = nullptr) override;
            ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
            ///
            /// \note Slot must belong to the current character.

            ///Saves a file, using supplied filename, overwritting if needed
            /** This is mostly used for quicksaving and autosaving, for they use the same name over and over again
                \param name Name of save, defaults to "Quicksave"**/
            void quickSave(std::string name = "Quicksave") override;

            ///Loads the last saved file
            /** Used for quickload **/
            void quickLoad() override;

            void loadGame (const std::string& filepath) override;
            ///< Load a saved game directly from the given file path. This will search the CharacterManager
            /// for a Character containing this save file, and set this Character current if one was found.
            /// Otherwise, a new Character will be created.

            void loadGame (const Character *character, const std::string &filepath) override;
            ///< Load a saved game file belonging to the given character.

            Character *getCurrentCharacter () override;
            ///< @note May return null.

            CharacterIterator characterBegin() override;
            ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
            /// iterator.

            CharacterIterator characterEnd() override;

            void update (float duration) override;
    };
}

#endif
