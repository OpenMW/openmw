#ifndef GAME_MWWORLD_LOCALSCRIPTS_H
#define GAME_MWWORLD_LOCALSCRIPTS_H

#include <list>
#include <string>

#include "ptr.hpp"

namespace MWWorld
{
    class ESMStore;
    class CellStore;
    class RefData;

    /// \brief List of active local scripts
    class LocalScripts
    {
            std::list<std::pair<std::string, Ptr> > mScripts;
            std::list<std::pair<std::string, Ptr> >::iterator mIter;
            const MWWorld::ESMStore& mStore;

        public:

            LocalScripts (const MWWorld::ESMStore& store);

            void startIteration();
            ///< Set the iterator to the begin of the script list.

            bool getNext(std::pair<std::string, Ptr>& script);
            ///< Get next local script
            /// @return Did we get a script?

            void add (const std::string& scriptName, const Ptr& ptr);
            ///< Add script to collection of active local scripts.

            void addCell (CellStore *cell);
            ///< Add all local scripts in a cell.

            void clear();
            ///< Clear active local scripts collection.

            void clearCell (CellStore *cell);
            ///< Remove all scripts belonging to \a cell.
            
            void remove (RefData *ref);

            void remove (const Ptr& ptr);
            ///< Remove script for given reference (ignored if reference does not have a script listed).
    };
}

#endif
