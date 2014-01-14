#ifndef GAME_MWWORLD_CELLS_H
#define GAME_MWWORLD_CELLS_H

#include <map>
#include <list>
#include <string>

#include "ptr.hpp"

namespace ESM
{
    class ESMReader;
}

namespace MWWorld
{
    class ESMStore;

    /// \brief Cell container
    class Cells
    {
            const MWWorld::ESMStore& mStore;
            std::vector<ESM::ESMReader>& mReader;
            std::map<std::string, CellStore> mInteriors;
            std::map<std::pair<int, int>, CellStore> mExteriors;
            std::vector<std::pair<std::string, CellStore *> > mIdCache;
            std::size_t mIdCacheIndex;

            Cells (const Cells&);
            Cells& operator= (const Cells&);

            CellStore *getCellStore (const ESM::Cell *cell);

            Ptr getPtrAndCache (const std::string& name, CellStore& cellStore);

        public:

            void clear();

            Cells (const MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& reader);
            ///< \todo pass the dynamic part of the ESMStore isntead (once it is written) of the whole
            /// world

            CellStore *getExterior (int x, int y);

            CellStore *getInterior (const std::string& name);

            Ptr getPtr (const std::string& name, CellStore& cellStore, bool searchInContainers = false);
            ///< \param searchInContainers Only affect loaded cells.
            /// @note name must be lower case

            /// @note name must be lower case
            Ptr getPtr (const std::string& name);

            /// Get all Ptrs referencing \a name in exterior cells
            /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
            /// @note name must be lower case
            void getExteriorPtrs (const std::string& name, std::vector<MWWorld::Ptr>& out);
    };
}

#endif
