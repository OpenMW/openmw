#include "character.hpp"

#include <cctype>
#include <sstream>

#include <boost/filesystem.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/defs.hpp>

bool MWState::operator< (const Slot& left, const Slot& right)
{
    return left.mTimeStamp<right.mTimeStamp;
}


void MWState::Character::addSlot (const boost::filesystem::path& path, const std::string& game)
{
    Slot slot;
    slot.mPath = path;
    slot.mTimeStamp = boost::filesystem::last_write_time (path);

    ESM::ESMReader reader;
    reader.open (slot.mPath.string());

    if (reader.getRecName()!=ESM::REC_SAVE)
        return; // invalid save file -> ignore

    reader.getRecHeader();

    slot.mProfile.load (reader);

    if (Misc::StringUtils::lowerCase (slot.mProfile.mContentFiles.at (0))!=
        Misc::StringUtils::lowerCase (game))
        return; // this file is for a different game -> ignore

    mSlots.push_back (slot);
}

void MWState::Character::addSlot (const ESM::SavedGame& profile)
{
    Slot slot;

    std::ostringstream stream;

    // The profile description is user-supplied, so we need to escape the path
    for (std::string::const_iterator it = profile.mDescription.begin(); it != profile.mDescription.end(); ++it)
    {
        if (std::isalnum(*it))  // Ignores multibyte characters and non alphanumeric characters
            stream << *it;
        else
            stream << "_";
    }

    const std::string ext = ".omwsave";
    slot.mPath = mPath / (stream.str() + ext);

    // Append an index if necessary to ensure a unique file
    int i=0;
    while (boost::filesystem::exists(slot.mPath))
    {
        const std::string test = stream.str() + " - " + std::to_string(++i);
        slot.mPath = mPath / (test + ext);
    }

    slot.mProfile = profile;
    slot.mTimeStamp = std::time (0);

    mSlots.push_back (slot);
}

MWState::Character::Character (const boost::filesystem::path& saves, const std::string& game)
: mPath (saves)
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
            boost::filesystem::path slotPath = *iter;

            try
            {
                addSlot (slotPath, game);
            }
            catch (...) {} // ignoring bad saved game files for now
        }

        std::sort (mSlots.begin(), mSlots.end());
    }
}

void MWState::Character::cleanup()
{
    if (mSlots.size() == 0)
    {
        // All slots are gone, no need to keep the empty directory
        if (boost::filesystem::is_directory (mPath))
        {
            // Extra safety check to make sure the directory is empty (e.g. slots failed to parse header)
            boost::filesystem::directory_iterator it(mPath);
            if (it == boost::filesystem::directory_iterator())
                boost::filesystem::remove_all(mPath);
        }
    }
}

const MWState::Slot *MWState::Character::createSlot (const ESM::SavedGame& profile)
{
    addSlot (profile);

    return &mSlots.back();
}

void MWState::Character::deleteSlot (const Slot *slot)
{
    int index = slot - &mSlots[0];

    if (index<0 || index>=static_cast<int> (mSlots.size()))
    {
        // sanity check; not entirely reliable
        throw std::logic_error ("slot not found");
    }

    boost::filesystem::remove(slot->mPath);

    mSlots.erase (mSlots.begin()+index);
}

const MWState::Slot *MWState::Character::updateSlot (const Slot *slot, const ESM::SavedGame& profile)
{
    int index = slot - &mSlots[0];

    if (index<0 || index>=static_cast<int> (mSlots.size()))
    {
        // sanity check; not entirely reliable
        throw std::logic_error ("slot not found");
    }

    Slot newSlot = *slot;
    newSlot.mProfile = profile;
    newSlot.mTimeStamp = std::time (0);

    mSlots.erase (mSlots.begin()+index);

    mSlots.push_back (newSlot);

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

ESM::SavedGame MWState::Character::getSignature() const
{
    if (mSlots.empty())
        throw std::logic_error ("character signature not available");

    std::vector<Slot>::const_iterator iter (mSlots.begin());

    Slot slot = *iter;

    for (++iter; iter!=mSlots.end(); ++iter)
        if (iter->mProfile.mPlayerLevel>slot.mProfile.mPlayerLevel)
            slot = *iter;
        else if (iter->mProfile.mPlayerLevel==slot.mProfile.mPlayerLevel &&
            iter->mTimeStamp>slot.mTimeStamp)
            slot = *iter;

    return slot.mProfile;
}

const boost::filesystem::path& MWState::Character::getPath() const
{
    return mPath;
}
