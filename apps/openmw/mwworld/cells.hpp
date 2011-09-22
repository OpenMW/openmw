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
    /// \brief Cell container
    class Cells
    {
            const ESMS::ESMStore& mStore;
            ESM::ESMReader& mReader;
            std::map<std::string, Ptr::CellStore> mInteriors;
            std::map<std::pair<int, int>, Ptr::CellStore> mExteriors;

            Cells (const Cells&);
            Cells& operator= (const Cells&);

        public:

            Cells (const ESMS::ESMStore& store, ESM::ESMReader& reader);

            Ptr::CellStore *getExterior (int x, int y);

            Ptr::CellStore *getInterior (const std::string& name);

            Ptr getPtr (const std::string& name, Ptr::CellStore& cellStore);
    };
}

#endif
