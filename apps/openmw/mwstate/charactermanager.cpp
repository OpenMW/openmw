
#include "charactermanager.hpp"

#include <sstream>
#include <stdexcept>

#include <boost/filesystem.hpp>

MWState::CharacterManager::CharacterManager (const boost::filesystem::path& saves,
    const std::string& game)
: mPath (saves), mNext (0), mCurrent (0), mGame (game)
{
    if (!boost::filesystem::is_directory (mPath))
    {
        boost::filesystem::create_directories (mPath);
    }
    else
    {
        for (boost::filesystem::directory_iterator iter (mPath);
            iter!=boost::filesystem::directory_iterator(); ++iter)
        {
            boost::filesystem::path characterDir = *iter;

            if (boost::filesystem::is_directory (characterDir))
            {
                Character character (characterDir, mGame);

                if (character.begin()!=character.end())
                    mCharacters.push_back (character);
            }

            std::istringstream stream (characterDir.filename().string());

            int index = 0;

            if ((stream >> index) && index>=mNext)
                mNext = index+1;
        }
    }
}

MWState::Character *MWState::CharacterManager::getCurrentCharacter (bool create)
{
    if (!mCurrent && create)
        createCharacter();

    return mCurrent;
}

void MWState::CharacterManager::deleteSlot(const MWState::Character *character, const MWState::Slot *slot)
{
    std::list<Character>::iterator it = findCharacter(character);

    it->deleteSlot(slot);

    if (character->begin() == character->end())
    {
        // All slots deleted, cleanup and remove this character
        it->cleanup();
        if (character == mCurrent)
            mCurrent = NULL;
        mCharacters.erase(it);
    }
}

void MWState::CharacterManager::createCharacter()
{
    std::ostringstream stream;
    stream << mNext++;

    boost::filesystem::path path = mPath / stream.str();

    mCharacters.push_back (Character (path, mGame));

    mCurrent = &mCharacters.back();
}

std::list<MWState::Character>::iterator MWState::CharacterManager::findCharacter(const MWState::Character* character)
{
    std::list<Character>::iterator it = mCharacters.begin();
    for (; it != mCharacters.end(); ++it)
    {
        if (&*it == character)
            break;
    }
    if (it == mCharacters.end())
        throw std::logic_error ("invalid character");
    return it;
}

void MWState::CharacterManager::setCurrentCharacter (const Character *character)
{
    std::list<Character>::iterator it = findCharacter(character);

    mCurrent = &*it;
}

void MWState::CharacterManager::clearCurrentCharacter()
{
    mCurrent = 0;
}

std::list<MWState::Character>::const_iterator MWState::CharacterManager::begin() const
{
    return mCharacters.begin();
}

std::list<MWState::Character>::const_iterator MWState::CharacterManager::end() const
{
    return mCharacters.end();
}
