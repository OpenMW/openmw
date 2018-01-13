#ifndef GAME_STATE_QUICKSAVEMANAGER_H
#define GAME_STATE_QUICKSAVEMANAGER_H

#include <string>

#include "character.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWState{
    class QuickSaveManager{
        std::string mSaveName;
        int mMaxSaves;
        int mSlotsVisited;
        int mOldestSlotId;
        const Slot *mOldestSlotVisited;
    private:
        bool tryExtractSlotId(const std::string &slotName, int &extractedIdll);
        bool isSlotIdValid(int slotId);
        bool shouldCreateNewSlot();
        bool isOldestSave(const Slot *compare);
        int calcNextSlotId();
    public:
        QuickSaveManager(std::string &saveName, int maxSaves);
        ///< A utility class to manage multiple quicksave slots
        ///
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)
        /// \param maxSaves The maximum number of save slots to use before recycling old ones

        void visitSave(const Slot *saveSlot);
        ///< Visits the given \a slot \a

        const Slot *getNextQuickSaveSlot();
        ///< Get the slot that the next quicksave should use.
        ///
        ///\return Either the oldest quicksave slot visited, or NULL if a new slot can be made

        std::string getNextQuickSaveName();
        ///< Get the name that the next quicksave should use ("QuickSave 1", "AutoSave 10", etc)
    };
}

#endif
