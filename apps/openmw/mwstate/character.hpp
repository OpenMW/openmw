#ifndef GAME_STATE_CHARACTER_H
#define GAME_STATE_CHARACTER_H

#include <filesystem>

#include <components/esm3/savedgame.hpp>

namespace MWState
{
    struct Slot
    {
        std::filesystem::path mPath;
        ESM::SavedGame mProfile;
        std::filesystem::file_time_type mTimeStamp;
    };

    bool operator<(const Slot& left, const Slot& right);

    std::string getFirstGameFile(const std::vector<std::string>& contentFiles);

    class Character
    {
    public:
        typedef std::vector<Slot>::const_reverse_iterator SlotIterator;

    private:
        std::filesystem::path mPath;
        std::vector<Slot> mSlots;

        void addSlot(const std::filesystem::path& path, const std::string& game);

        void addSlot(const ESM::SavedGame& profile);

    public:
        Character(const std::filesystem::path& saves, const std::string& game);

        void cleanup();
        ///< Delete the directory we used, if it is empty

        const Slot* createSlot(const ESM::SavedGame& profile);
        ///< Create new slot.
        ///
        /// \attention The ownership of the slot is not transferred.

        /// \note Slot must belong to this character.
        ///
        /// \attention The \a slot pointer will be invalidated by this call.
        void deleteSlot(const Slot* slot);

        const Slot* updateSlot(const Slot* slot, const ESM::SavedGame& profile);
        /// \note Slot must belong to this character.
        ///
        /// \attention The \a slot pointer will be invalidated by this call.

        SlotIterator begin() const;
        ///<  Any call to createSlot and updateSlot can invalidate the returned iterator.

        SlotIterator end() const;

        const std::filesystem::path& getPath() const;

        const ESM::SavedGame& getSignature() const;
        ///< Return signature information for this character.
        ///
        /// \attention This function must not be called if there are no slots.
    };
}

#endif
