#ifndef OPENMW_CELLCONTROLLER_HPP
#define OPENMW_CELLCONTROLLER_HPP

#include "../mwworld/cellstore.hpp"

namespace mwmp
{
    class CellController
    {
    public:

        CellController();
        ~CellController();

        int getCellSize() const;
        
        virtual MWWorld::CellStore *getCell(const ESM::Cell& cell);

        void openContainer(const MWWorld::Ptr& container,  bool loot);
        void closeContainer(const MWWorld::Ptr& container);
    };
}

#endif //OPENMW_CELLCONTROLLER_HPP
