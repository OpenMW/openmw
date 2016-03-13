#ifndef GAME_STATE_CHARACTERMANAGER_H
#define GAME_STATE_CHARACTERMANAGER_H

#include <boost/filesystem/path.hpp>

#include "character.hpp"

namespace MWState
{
    class CharacterManager
    {
            boost::filesystem::path mPath;

            // Uses std::list, so that mCurrent stays valid when characters are deleted
            std::list<Character> mCharacters;

            Character *mCurrent;
            std::string mGame;

        private:

            CharacterManager (const CharacterManager&);
            ///< Not implemented

            CharacterManager& operator= (const CharacterManager&);
            ///< Not implemented

            std::list<Character>::iterator findCharacter(const MWState::Character* character);

        public:

            CharacterManager (const boost::filesystem::path& saves, const std::string& game);

            Character *getCurrentCharacter ();
            ///< @note May return null

            void deleteSlot(const MWState::Character *character, const MWState::Slot *slot);

            Character* createCharacter(const std::string& name);
            ///< Create new character within saved game management
            /// \param name Name for the character (does not need to be unique)

            void setCurrentCharacter (const Character *character);

            std::list<Character>::const_iterator begin() const;

            std::list<Character>::const_iterator end() const;
    };
}

#endif
