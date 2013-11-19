#ifndef GAME_STATE_CHARACTER_H
#define GAME_STATE_CHARACTER_H

#include <boost/filesystem/path.hpp>

#include <components/esm/savedgame.hpp>

namespace MWState
{
    struct Slot
    {
        boost::filesystem::path mPath;
        ESM::SavedGame mProfile;
        std::time_t mTimeStamp;
    };

    bool operator< (const Slot& left, const Slot& right);

    class Character
    {
            boost::filesystem::path mPath;
            std::vector<Slot> mSlots;
            int mNext;

            void addSlot (const boost::filesystem::path& path);

            void addSlot (const ESM::SavedGame& profile);

        public:

            Character (const boost::filesystem::path& saves);

            const Slot *createSlot (const ESM::SavedGame& profile);
            ///< Create new slot.
            ///
            /// \attention The ownership of the slot is not transferred.

            const Slot *updateSlot (const Slot *slot, const ESM::SavedGame& profile);
            /// \note Slot must belong to this character.
            ///
            /// \attention The \æ slot pointer will be invalidated by this call.
    };
}

#endif
