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
            ///< Must not be called, if there is no current character.

            void createCharacter();
            ///< Create new character within saved game management
    };
}

#endif
