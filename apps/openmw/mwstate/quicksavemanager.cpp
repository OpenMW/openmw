#include "quicksavemanager.hpp"

namespace MWState {
    class NextSlotFinder {
        std::string mSaveName;
        unsigned int mMaxSaves;
        unsigned int mSlotsVisited;
        const Slot *mOldestSlotVisited;
    private:
        bool shouldCreateNewSlot() const;
        bool isOldestSave(const Slot *compare) const;
    public:
        NextSlotFinder(const std::string &saveName, unsigned int maxSaves);
        ///< A utility class to manage multiple quicksave slots
        ///
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)
        /// \param maxSaves The maximum number of save slots to create before recycling old ones

        void visitSave(const Slot *saveSlot);
        ///< Visits the given \a slot \a

        const Slot *getNextQuickSaveSlot() const;
        ///< Get the slot that the next quicksave should use.
        ///
        ///\return Either the oldest quicksave slot visited, or NULL if a new slot can be made
    };
}

MWState::NextSlotFinder::NextSlotFinder(const std::string &saveName, unsigned int maxSaves)
  : mSaveName(saveName)
  , mMaxSaves(maxSaves)
  , mSlotsVisited(0)
  , mOldestSlotVisited(nullptr)
{
}

void MWState::NextSlotFinder::visitSave(const Slot *saveSlot)
{
    if(mSaveName == saveSlot->mProfile.mDescription)
    {
        ++mSlotsVisited;
        if(isOldestSave(saveSlot))
            mOldestSlotVisited = saveSlot;
    }
}

bool MWState::NextSlotFinder::isOldestSave(const Slot *compare) const
{
    if(mOldestSlotVisited == NULL)
        return true;
    return (compare->mTimeStamp <= mOldestSlotVisited->mTimeStamp);
}

bool MWState::NextSlotFinder::shouldCreateNewSlot() const
{
    return (mSlotsVisited < mMaxSaves);
}

const MWState::Slot *MWState::NextSlotFinder::getNextQuickSaveSlot() const
{
    if(shouldCreateNewSlot())
        return NULL;
    return mOldestSlotVisited;
}


namespace MWState {
    class LatestSlotFinder {
        std::string mSaveName;
        const Slot *mLatestSlotVisited;
    private:
        bool isLatestSave(const Slot *compare) const;
    public:
        LatestSlotFinder(const std::string &saveName);
        ///< A utility class to manage multiple quicksave slots
        ///
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)

        void visitSave(const Slot *saveSlot);
        ///< Visits the given \a slot \a

        const Slot *getLatestSaveSlot() const;
        ///< Get the latest quicksave.
        ///
        ///\return Either the latest quicksave slot visited, or NULL if there is no quicksave
    };
}

MWState::LatestSlotFinder::LatestSlotFinder(const std::string &saveName)
    : mSaveName(saveName)
    , mLatestSlotVisited(nullptr)
{
}

void MWState::LatestSlotFinder::visitSave(const Slot *saveSlot)
{
    if (mSaveName == saveSlot->mProfile.mDescription)
    {
        if (isLatestSave(saveSlot))
            mLatestSlotVisited = saveSlot;
    }
}

bool MWState::LatestSlotFinder::isLatestSave(const Slot *compare) const
{
    if (mLatestSlotVisited == nullptr)
        return true;
    return (compare->mTimeStamp > mLatestSlotVisited->mTimeStamp);
}

const MWState::Slot *MWState::LatestSlotFinder::getLatestSaveSlot() const
{
    return mLatestSlotVisited;
}

const MWState::Slot *MWState::QuickSaveManager::findNextQuickSaveSlot(const MWState::Character &character, const std::string &saveName, unsigned int maxSaves)
{
    NextSlotFinder saveFinder(saveName, maxSaves);
    for (const Slot &save : character)
        saveFinder.visitSave(&save);
    return saveFinder.getNextQuickSaveSlot();
}

const MWState::Slot *MWState::QuickSaveManager::findLatestSaveSlot(const MWState::Character &character, const std::string &saveName)
{
    LatestSlotFinder saveFinder(saveName);
    for (const Slot &save : character)
        saveFinder.visitSave(&save);
    return saveFinder.getLatestSaveSlot();
}
