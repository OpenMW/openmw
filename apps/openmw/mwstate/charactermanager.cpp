#include "charactermanager.hpp"

#include <cctype>
#include <filesystem>
#include <sstream>
#include <utility>

#include <components/misc/utf8stream.hpp>

MWState::CharacterManager::CharacterManager(std::filesystem::path saves, const std::vector<std::string>& contentFiles)
    : mPath(std::move(saves))
    , mCurrent(nullptr)
    , mGame(getFirstGameFile(contentFiles))
{
    if (!std::filesystem::is_directory(mPath))
    {
        std::filesystem::create_directories(mPath);
    }
    else
    {
        for (const std::filesystem::path& characterDir : std::filesystem::directory_iterator(mPath))
        {
            if (std::filesystem::is_directory(characterDir))
            {
                Character character(characterDir, mGame);

                if (character.begin() != character.end())
                    mCharacters.push_back(std::move(character));
            }
        }
        mCharacters.sort();
    }
}

MWState::Character* MWState::CharacterManager::getCurrentCharacter()
{
    return mCurrent;
}

void MWState::CharacterManager::deleteSlot(const MWState::Slot* slot, const MWState::Character*& character)
{
    std::list<Character>::iterator it = findCharacter(character);

    it->deleteSlot(slot);

    if (character->begin() == character->end())
    {
        // All slots deleted, cleanup and remove this character
        it->cleanup();
        if (character == mCurrent)
            mCurrent = nullptr;
        mCharacters.erase(it);
        character = nullptr;
    }
}

MWState::Character* MWState::CharacterManager::createCharacter(const std::string& name)
{
    std::ostringstream stream;

    // The character name is user-supplied, so we need to escape the path
    Utf8Stream nameStream(name);
    while (!nameStream.eof())
    {
        auto c = nameStream.consume();
        if (c <= 0x7F && std::isalnum(c)) // Ignore multibyte characters and non alphanumeric characters
            stream << static_cast<char>(c);
        else
            stream << '_';
    }

    std::filesystem::path path = mPath / stream.str();

    // Append an index if necessary to ensure a unique directory
    int i = 0;
    while (std::filesystem::exists(path))
    {
        std::ostringstream test;
        test << stream.str();
        test << " - " << ++i;
        path = mPath / test.str();
    }

    return &mCharacters.emplace_front(path, mGame);
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
        throw std::logic_error("invalid character");
    return it;
}

void MWState::CharacterManager::setCurrentCharacter(const Character* character)
{
    if (!character)
        mCurrent = nullptr;
    else
    {
        std::list<Character>::iterator it = findCharacter(character);

        mCurrent = &*it;
    }
}

std::list<MWState::Character>::const_iterator MWState::CharacterManager::begin() const
{
    return mCharacters.begin();
}

std::list<MWState::Character>::const_iterator MWState::CharacterManager::end() const
{
    return mCharacters.end();
}
