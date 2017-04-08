#ifndef OPENMW_CELL_HPP
#define OPENMW_CELL_HPP

#include "WorldEvent.hpp"
#include "LocalActor.hpp"
#include "DedicatedActor.hpp"
#include "../mwworld/cellstore.hpp"

namespace mwmp
{
    class Cell
    {
    public:

        Cell(MWWorld::CellStore* cellStore);
        ~Cell();

        void updateLocal();
        void updateDedicated(float dt);

        void initializeLocalActors();
        void uninitializeLocalActors();
        void uninitializeDedicatedActors();

        void readCellFrame(mwmp::WorldEvent& worldEvent);

        virtual LocalActor *getLocalActor(std::string actorIndex);
        virtual DedicatedActor *getDedicatedActor(std::string actorIndex);

        MWWorld::CellStore* getCellStore();
        std::string getDescription();

    private:
        MWWorld::CellStore* store;
        std::map<std::string, mwmp::LocalActor *> localActors;
        std::map<std::string, mwmp::DedicatedActor *> dedicatedActors;
    };
}

#endif //OPENMW_CELL_HPP
