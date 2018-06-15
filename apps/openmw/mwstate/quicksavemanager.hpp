#ifndef GAME_STATE_QUICKSAVEMANAGER_H
#define GAME_STATE_QUICKSAVEMANAGER_H

#include <string>

#include "character.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWState{
    class NextSlotFinder{
        std::string mSaveName;
        unsigned int mMaxSaves;
        unsigned int mSlotsVisited;
        const Slot *mOldestSlotVisited;
    private:
        bool shouldCreateNewSlot();
        bool isOldestSave(const Slot *compare);
    public:
        NextSlotFinder(std::string &saveName, unsigned int maxSaves);
        ///< A utility class to find the next usable quicksave slots
        ///
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)
        /// \param maxSaves The maximum number of save slots to create before recycling old ones

        void visitSave(const Slot *saveSlot);
        ///< Visits the given \a slot \a

        const Slot *getNextQuickSaveSlot();
        ///< Get the slot that the next quicksave should use.
        ///
        ///\return Either the oldest quicksave slot visited, or NULL if a new slot can be made
    };

    class LatestSlotFinder {
        std::string mSaveName;
        const Slot *mLatestSlotVisited;
    private:
        bool isLatestSave(const Slot *compare) const;
    public:
        LatestSlotFinder(const std::string &saveName);
        ///< A utility class to find the latest quicksave slots
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

#endif
