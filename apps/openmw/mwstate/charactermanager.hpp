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

        private:

            CharacterManager (const CharacterManager&);
            ///< Not implemented

            CharacterManager& operator= (const CharacterManager&);
            ///< Not implemented

        public:

            CharacterManager (const boost::filesystem::path& saves);

            Character *getCurrentCharacter();
            ///< A character is implicitly created, if there is none.

            void createCharacter();
            ///< Create new character within saved game management

            void clearCurrentCharacter();

            std::vector<Character>::const_iterator begin() const;

            std::vector<Character>::const_iterator end() const;
    };
}

#endif
