#ifndef GAME_STATE_QUICKSAVEMANAGER_H
#define GAME_STATE_QUICKSAVEMANAGER_H

#include <string>

#include "character.hpp"
#include "../mwbase/statemanager.hpp"

namespace MWState{
    namespace QuickSaveManager {
        const Slot *findNextQuickSaveSlot(const Character &character, const std::string &saveName, unsigned int maxSaves);
        ///< Get the slot that the next quicksave should use.
        ///
        /// \param character The character owner of the slots
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)
        /// \param maxSaves The maximum number of save slots to create before recycling old ones
        ///\return Either the oldest quicksave slot visited, or NULL if a new slot can be made

        const Slot *findLatestSaveSlot(const Character &character, const std::string &saveName);
        ///< Get the latest quicksave.
        ///
        /// \param character The character owner of the slots
        /// \param saveName The name of the save ("QuickSave", "AutoSave", etc)
        ///\return Either the latest quicksave slot visited, or NULL if there is no quicksave
    }
}

#endif
