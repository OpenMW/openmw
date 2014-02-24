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
        public:

            typedef std::vector<Slot>::const_reverse_iterator SlotIterator;

        private:

            boost::filesystem::path mPath;
            std::vector<Slot> mSlots;
            int mNext;

            void addSlot (const boost::filesystem::path& path, const std::string& game);

            void addSlot (const ESM::SavedGame& profile);

        public:

            Character (const boost::filesystem::path& saves, const std::string& game);

            const Slot *createSlot (const ESM::SavedGame& profile);
            ///< Create new slot.
            ///
            /// \attention The ownership of the slot is not transferred.

            const Slot *updateSlot (const Slot *slot, const ESM::SavedGame& profile);
            /// \note Slot must belong to this character.
            ///
            /// \attention The \a slot pointer will be invalidated by this call.

            SlotIterator begin() const;
            ///< First slot is the most recent. Other slots follow in descending order of save date.
            ///
            /// Any call to createSlot and updateSlot can invalidate the returned iterator.

            SlotIterator end() const;

            ESM::SavedGame getSignature() const;
            ///< Return signature information for this character.
            ///
            /// \attention This function must not be called if there are no slots.
    };
}

#endif
