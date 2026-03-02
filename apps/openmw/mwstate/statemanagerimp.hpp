#ifndef GAME_STATE_STATEMANAGER_H
#define GAME_STATE_STATEMANAGER_H

#include <filesystem>
#include <map>
#include <utility>

#include "../mwbase/statemanager.hpp"

#include "charactermanager.hpp"

namespace MWState
{
    class StateManager : public MWBase::StateManager
    {
        bool mQuitRequest;
        bool mAskLoadRecent;
        bool mNewGameRequest = false;
        std::optional<std::pair<const Character*, std::filesystem::path>> mLoadRequest;
        State mState;
        CharacterManager mCharacterManager;
        double mTimePlayed;
        std::filesystem::path mLastSavegame;

    private:
        void cleanup(bool force = false);

        void printSavegameFormatError(const std::string& exceptionText, const std::string& messageBoxText);

        bool confirmLoading(const std::vector<std::string_view>& missingFiles) const;

        void writeScreenshot(std::vector<char>& imageData) const;

        std::map<int, int> buildContentFileIndexMap(const ESM::ESMReader& reader) const;

    public:
        StateManager(const std::filesystem::path& saves, const std::vector<std::string>& contentFiles);

        void requestQuit() override;

        bool hasQuitRequest() const override;

        void askLoadRecent() override;

        void requestNewGame() override { mNewGameRequest = true; }
        void requestLoad(const Character* character, const std::filesystem::path& filepath) override
        {
            mLoadRequest.emplace(character, filepath);
        }

        State getState() const override;

        void newGame(bool bypass = false) override;
        ///< Start a new game.
        ///
        /// \param bypass Skip new game mechanics.

        void endGame();

        void resumeGame() override;

        void deleteGame(const MWState::Character* character, const MWState::Slot* slot) override;
        ///< Delete a saved game slot from this character. If all save slots are deleted, the character will be deleted
        ///< too.

        void saveGame(std::string_view description, const Slot* slot = nullptr) override;
        ///< Write a saved game to \a slot or create a new slot if \a slot == 0.
        ///
        /// \note Slot must belong to the current character.

        /// Saves a file, using supplied filename, overwritting if needed
        /** This is mostly used for quicksaving and autosaving, for they use the same name over and over again
            \param name Name of save, defaults to "Quicksave"**/
        void quickSave(std::string name = "Quicksave") override;

        /// Loads the last saved file
        /** Used for quickload **/
        void quickLoad() override;

        void loadGame(const std::filesystem::path& filepath) override;
        ///< Load a saved game directly from the given file path. This will search the CharacterManager
        /// for a Character containing this save file, and set this Character current if one was found.
        /// Otherwise, a new Character will be created.

        void loadGame(const Character* character, const std::filesystem::path& filepath) override;
        ///< Load a saved game file belonging to the given character.

        Character* getCurrentCharacter() override;
        ///< @note May return null.

        CharacterIterator characterBegin() override;
        ///< Any call to SaveGame and getCurrentCharacter can invalidate the returned
        /// iterator.

        CharacterIterator characterEnd() override;

        void update(float duration);
    };
}

#endif
