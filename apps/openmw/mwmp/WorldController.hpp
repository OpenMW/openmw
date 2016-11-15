#ifndef OPENMW_WORLDCONTROLLER_HPP
#define OPENMW_WORLDCONTROLLER_HPP

namespace mwmp
{
    class WorldController
    {
    public:

        WorldController();
        ~WorldController();

        virtual MWWorld::CellStore *getCell(const ESM::Cell& cell);
    };
}

#endif //OPENMW_WORLDCONTROLLER_HPP
