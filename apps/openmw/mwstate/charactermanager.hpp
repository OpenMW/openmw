#ifndef GAME_STATE_CHARACTERMANAGER_H
#define GAME_STATE_CHARACTERMANAGER_H

#include <boost/filesystem/path.hpp>

#include "character.hpp"

namespace MWState
{
    class CharacterManager
    {
            boost::filesystem::path mPath;
            int mNext;
            std::vector<Character> mCharacters;
            Character *mCurrent;
            std::string mGame;

        private:

            CharacterManager (const CharacterManager&);
            ///< Not implemented

            CharacterManager& operator= (const CharacterManager&);
            ///< Not implemented

        public:

            CharacterManager (const boost::filesystem::path& saves, const std::string& game);

            Character *getCurrentCharacter (bool create = true);
            ///< \param create Create a new character, if there is no current character.

            void deleteSlot(const MWState::Character *character, const MWState::Slot *slot);

            void createCharacter();
            ///< Create new character within saved game management

            void setCurrentCharacter (const Character *character);

            void clearCurrentCharacter();

            std::vector<Character>::const_iterator begin() const;

            std::vector<Character>::const_iterator end() const;
    };
}

#endif
