#include "character.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <tuple>
#include <utility>

#include <components/debug/debuglog.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/utf8stream.hpp>

bool MWState::operator<(const Slot& left, const Slot& right)
{
    return left.mTimeStamp < right.mTimeStamp;
}

bool MWState::operator<(const Character& left, const Character& right)
{
    if (left.mSlots.empty() && right.mSlots.empty())
        return left.mPath < right.mPath;
    else if (left.mSlots.empty())
        return false;
    else if (right.mSlots.empty())
        return true;
    return right.mSlots.back() < left.mSlots.back();
}

std::string_view MWState::getFirstGameFile(const std::vector<std::string>& contentFiles)
{
    for (const std::string& c : contentFiles)
    {
        if (Misc::StringUtils::ciEndsWith(c, ".esm") || Misc::StringUtils::ciEndsWith(c, ".omwgame"))
            return c;
    }
    return {};
}

void MWState::Character::addSlot(const std::filesystem::path& path, const std::string& game)
{
    Slot slot;
    slot.mPath = path;
    slot.mTimeStamp = std::filesystem::last_write_time(path);

    ESM::ESMReader reader;
    reader.open(slot.mPath);

    if (reader.getRecName() != ESM::REC_SAVE)
        return; // invalid save file -> ignore

    reader.getRecHeader();

    slot.mProfile.load(reader);

    if (!Misc::StringUtils::ciEqual(getFirstGameFile(slot.mProfile.mContentFiles), game))
        return; // this file is for a different game -> ignore

    mSlots.push_back(std::move(slot));
}

void MWState::Character::addSlot(const ESM::SavedGame& profile)
{
    Slot slot;

    std::ostringstream stream;

    // The profile description is user-supplied, so we need to escape the path
    Utf8Stream description(profile.mDescription);
    while (!description.eof())
    {
        auto c = description.consume();
        if (c <= 0x7F && std::isalnum(c)) // Ignore multibyte characters and non alphanumeric characters
            stream << static_cast<char>(c);
        else
            stream << '_';
    }

    const std::string ext = ".omwsave";
    slot.mPath = mPath / (stream.str() + ext);

    // Append an index if necessary to ensure a unique file
    int i = 0;
    while (std::filesystem::exists(slot.mPath))
    {
        const std::string test = stream.str() + " - " + std::to_string(++i);
        slot.mPath = mPath / (test + ext);
    }

    slot.mProfile = profile;
    slot.mTimeStamp = std::filesystem::file_time_type::clock::now();

    mSlots.push_back(std::move(slot));
}

MWState::Character::Character(const std::filesystem::path& saves, const std::string& game)
    : mPath(saves)
{
    if (!std::filesystem::is_directory(mPath))
    {
        std::filesystem::create_directories(mPath);
    }
    else
    {
        for (const auto& iter : std::filesystem::directory_iterator(mPath))
        {
            try
            {
                addSlot(iter, game);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Warning) << "Failed to add slot for game \"" << game << "\" save " << iter << ": "
                                    << e.what();
            }
        }

        std::sort(mSlots.begin(), mSlots.end());
    }
}

void MWState::Character::cleanup()
{
    if (mSlots.empty())
    {
        // All slots are gone, no need to keep the empty directory
        if (std::filesystem::is_directory(mPath))
        {
            // Extra safety check to make sure the directory is empty (e.g. slots failed to parse header)
            std::filesystem::directory_iterator it(mPath);
            if (it == std::filesystem::directory_iterator())
                std::filesystem::remove_all(mPath);
        }
    }
}

const MWState::Slot* MWState::Character::createSlot(const ESM::SavedGame& profile)
{
    addSlot(profile);

    return &mSlots.back();
}

void MWState::Character::deleteSlot(const Slot* slot)
{
    std::ptrdiff_t index = slot - mSlots.data();

    if (index < 0 || static_cast<std::size_t>(index) >= mSlots.size())
    {
        // sanity check; not entirely reliable
        throw std::logic_error("slot not found");
    }

    std::filesystem::remove(slot->mPath);

    mSlots.erase(mSlots.begin() + index);
}

const MWState::Slot* MWState::Character::updateSlot(const Slot* slot, const ESM::SavedGame& profile)
{
    std::ptrdiff_t index = slot - mSlots.data();

    if (index < 0 || static_cast<std::size_t>(index) >= mSlots.size())
    {
        // sanity check; not entirely reliable
        throw std::logic_error("slot not found");
    }

    Slot newSlot = *slot;
    newSlot.mProfile = profile;
    newSlot.mTimeStamp = std::filesystem::file_time_type::clock::now();

    mSlots.erase(mSlots.begin() + index);

    mSlots.push_back(std::move(newSlot));

    return &mSlots.back();
}

MWState::Character::SlotIterator MWState::Character::begin() const
{
    return mSlots.rbegin();
}

MWState::Character::SlotIterator MWState::Character::end() const
{
    return mSlots.rend();
}

const ESM::SavedGame& MWState::Character::getSignature() const
{
    if (mSlots.empty())
        throw std::logic_error("character signature not available");

    const auto tiePlayerLevelAndTimeStamp
        = [](const Slot& v) { return std::tie(v.mProfile.mPlayerLevel, v.mTimeStamp); };

    const auto lessByPlayerLevelAndTimeStamp
        = [&](const Slot& l, const Slot& r) { return tiePlayerLevelAndTimeStamp(l) < tiePlayerLevelAndTimeStamp(r); };

    return std::max_element(mSlots.begin(), mSlots.end(), lessByPlayerLevelAndTimeStamp)->mProfile;
}

const std::filesystem::path& MWState::Character::getPath() const
{
    return mPath;
}
