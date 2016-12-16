#ifndef OPENMW_WORLDCONTROLLER_HPP
#define OPENMW_WORLDCONTROLLER_HPP

#include "../mwworld/cellstore.hpp"

namespace mwmp
{
    class WorldController
    {
    public:

        WorldController();
        ~WorldController();

        void openContainer(const MWWorld::Ptr& container,  bool loot);
        void closeContainer(const MWWorld::Ptr& container);

        virtual MWWorld::CellStore *getCell(const ESM::Cell& cell);
    };
}

#endif //OPENMW_WORLDCONTROLLER_HPP
