#ifndef OPENMW_CELLCONTROLLER_HPP
#define OPENMW_CELLCONTROLLER_HPP

#include "Cell.hpp"
#include "ActorList.hpp"
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

        void updateLocal(bool forceUpdate);
        void updateDedicated(float dt);

        void initializeCell(const ESM::Cell& cell);
        void initializeLocalActors(const ESM::Cell& cell);
        void readPositions(mwmp::ActorList& actorList);

        void setLocalActorRecord(std::string actorIndex, std::string cellIndex);
        void removeLocalActorRecord(std::string actorIndex);
        bool isLocalActor(MWWorld::Ptr ptr);
        virtual LocalActor *getLocalActor(MWWorld::Ptr ptr);

        void setDedicatedActorRecord(std::string actorIndex, std::string cellIndex);
        void removeDedicatedActorRecord(std::string actorIndex);
        bool isDedicatedActor(MWWorld::Ptr ptr);
        virtual DedicatedActor *getDedicatedActor(MWWorld::Ptr ptr);

        std::string generateMapIndex(MWWorld::Ptr ptr);
        std::string generateMapIndex(mwmp::BaseActor baseActor);

        int getCellSize() const;
        virtual MWWorld::CellStore *getCellStore(const ESM::Cell& cell);

        void openContainer(const MWWorld::Ptr& container,  bool loot);
        void closeContainer(const MWWorld::Ptr& container);

    private:
        static std::map<std::string, mwmp::Cell *> cellsActive;
        static std::map<std::string, std::string> localActorsToCells;
        static std::map<std::string, std::string> dedicatedActorsToCells;
    };
}

#endif //OPENMW_CELLCONTROLLER_HPP
