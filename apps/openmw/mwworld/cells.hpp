#ifndef GAME_MWWORLD_CELLS_H
#define GAME_MWWORLD_CELLS_H

#include <map>
#include <list>
#include <string>

#include "ptr.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    struct CellId;
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

            void writeCell (ESM::ESMWriter& writer, const CellStore& cell) const;

            bool hasState (const CellStore& cellStore) const;
            ///< Check if cell has state that needs to be included in a saved game file.

        public:

            void clear();

            Cells (const MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& reader);

            CellStore *getExterior (int x, int y);

            CellStore *getInterior (const std::string& name);

            CellStore *getCell (const ESM::CellId& id);

            Ptr getPtr (const std::string& name, CellStore& cellStore, bool searchInContainers = false);
            ///< \param searchInContainers Only affect loaded cells.
            /// @note name must be lower case

            /// @note name must be lower case
            Ptr getPtr (const std::string& name);

            /// Get all Ptrs referencing \a name in exterior cells
            /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
            /// @note name must be lower case
            void getExteriorPtrs (const std::string& name, std::vector<MWWorld::Ptr>& out);

            /// Get all Ptrs referencing \a name in interior cells
            /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
            /// @note name must be lower case
            void getInteriorPtrs (const std::string& name, std::vector<MWWorld::Ptr>& out);

            int countSavedGameRecords() const;

            void write (ESM::ESMWriter& writer) const;

            bool readRecord (ESM::ESMReader& reader, int32_t type);
    };
}

#endif
