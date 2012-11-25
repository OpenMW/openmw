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

            void fillContainers (CellStore& cellStore);

            Ptr getPtrAndCache (const std::string& name, CellStore& cellStore);

        public:

            Cells (const MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& reader);
            ///< \todo pass the dynamic part of the ESMStore isntead (once it is written) of the whole
            /// world

            CellStore *getExterior (int x, int y);

            CellStore *getInterior (const std::string& name);

            Ptr getPtr (const std::string& name, CellStore& cellStore);

            Ptr getPtr (const std::string& name);
    };
}

#endif
