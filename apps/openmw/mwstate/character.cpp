
#include "character.hpp"

#include <ctime>

#include <sstream>
#include <algorithm>
#include <stdexcept>

#include <boost/filesystem.hpp>

bool MWState::operator< (const Slot& left, const Slot& right)
{
    return left.mTimeStamp<right.mTimeStamp;
}


void MWState::Character::addSlot (const boost::filesystem::path& path)
{
    Slot slot;
    slot.mPath = path;
    slot.mTimeStamp = boost::filesystem::last_write_time (path);

    /// \todo load profile

    mSlots.push_back (slot);
}

void MWState::Character::addSlot (const ESM::SavedGame& profile)
{
    Slot slot;

    std::ostringstream stream;
    stream << mNext++;

    slot.mPath = mPath / stream.str();
    slot.mProfile = profile;
    slot.mTimeStamp = std::time (0);

    mSlots.push_back (slot);
}

MWState::Character::Character (const boost::filesystem::path& saves)
: mPath (saves), mNext (0)
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
                addSlot (slotPath);
            }
            catch (...) {} // ignoring bad saved game files for now

            std::istringstream stream (slotPath.filename().string());

            int index = 0;

            if ((stream >> index) && index>=mNext)
                mNext = index+1;
        }

        std::sort (mSlots.begin(), mSlots.end());
    }
}

const MWState::Slot *MWState::Character::createSlot (const ESM::SavedGame& profile)
{
    addSlot (profile);

    return &mSlots.back();
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