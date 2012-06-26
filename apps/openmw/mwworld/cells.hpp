#ifndef GAME_MWWORLD_CELLS_H
#define GAME_MWWORLD_CELLS_H

#include <map>
#include <string>

#include "ptr.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESM
{
    class ESMStore;
}

namespace MWWorld
{
    class World;

    /// \brief Cell container
    class Cells
    {
            const ESMS::ESMStore& mStore;
            ESM::ESMReader& mReader;
            std::map<std::string, Ptr::CellStore> mInteriors;
            std::map<std::pair<int, int>, Ptr::CellStore> mExteriors;
            MWWorld::World& mWorld;
            std::vector<std::pair<std::string, Ptr::CellStore *> > mIdCache;
            std::size_t mIdCacheIndex;

            Cells (const Cells&);
            Cells& operator= (const Cells&);

            Ptr::CellStore *getCellStore (const ESM::Cell *cell);

            void fillContainers (Ptr::CellStore& cellStore);

            Ptr getPtrAndCache (const std::string& name, Ptr::CellStore& cellStore);

        public:

            Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader, MWWorld::World& world);
            ///< \todo pass the dynamic part of the ESMStore isntead (once it is written) of the whole
            /// world

            Ptr::CellStore *getExterior (int x, int y);

            Ptr::CellStore *getInterior (const std::string& name);

            Ptr getPtr (const std::string& name, Ptr::CellStore& cellStore);

            Ptr getPtr (const std::string& name);
    };
}

#endif
