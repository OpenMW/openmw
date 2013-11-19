
#include "charactermanager.hpp"

#include <sstream>
#include <stdexcept>

#include <boost/filesystem.hpp>

MWState::CharacterManager::CharacterManager (const boost::filesystem::path& saves)
: mPath (saves), mNext (0), mCurrent (0)
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
                Character character (characterDir);
                mCharacters.push_back (character);
            }

            std::istringstream stream (characterDir.filename().string());

            int index = 0;

            if ((stream >> index) && index>=mNext)
                mNext = index+1;
        }
    }
}

MWState::Character *MWState::CharacterManager::getCurrentCharacter()
{
    if (!mCurrent)
        throw std::logic_error ("no character selected");

    return mCurrent;
}

void MWState::CharacterManager::createCharacter()
{
    std::ostringstream stream;
    stream << mNext++;

    boost::filesystem::path path = mPath / stream.str();

    mCharacters.push_back (Character (path));

    mCurrent = &mCharacters.back();
}