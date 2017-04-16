#ifndef OPENMW_CELL_HPP
#define OPENMW_CELL_HPP

#include "ActorList.hpp"
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

        void updateLocal(bool forceUpdate);
        void updateDedicated(float dt);

        void readPositions(ActorList& actorList);
        void readAnimFlags(ActorList& actorList);
        void readAnimPlay(ActorList& actorList);
        void readStatsDynamic(ActorList& actorList);

        void initializeLocalActors();
        void initializeDedicatedActors(ActorList& actorList);

        void uninitializeLocalActors();
        void uninitializeDedicatedActors();

        virtual LocalActor *getLocalActor(std::string actorIndex);
        virtual DedicatedActor *getDedicatedActor(std::string actorIndex);

        MWWorld::CellStore* getCellStore();
        std::string getDescription();

    private:
        MWWorld::CellStore* store;
        std::map<std::string, LocalActor *> localActors;
        std::map<std::string, DedicatedActor *> dedicatedActors;
    };
}

#endif //OPENMW_CELL_HPP
