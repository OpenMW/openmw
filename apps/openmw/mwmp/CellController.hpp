#ifndef OPENMW_CELLCONTROLLER_HPP
#define OPENMW_CELLCONTROLLER_HPP

#include "Cell.hpp"
#include "WorldEvent.hpp"
#include "LocalActor.hpp"
#include "DedicatedActor.hpp"
#include "../mwworld/cellstore.hpp"

namespace mwmp
{
    class CellController
    {
    public:

        CellController();
        ~CellController();

        void updateLocal();
        void initializeCellLocal(const ESM::Cell& cell);
        void readCellFrame(mwmp::WorldEvent& worldEvent);

        int getCellSize() const;
        virtual MWWorld::CellStore *getCell(const ESM::Cell& cell);

        void openContainer(const MWWorld::Ptr& container,  bool loot);
        void closeContainer(const MWWorld::Ptr& container);

    private:
        static std::deque<mwmp::Cell *> cellsActive;
    };
}

#endif //OPENMW_CELLCONTROLLER_HPP
