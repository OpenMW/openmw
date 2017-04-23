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
        void readAnimFlags(mwmp::ActorList& actorList);
        void readAnimPlay(mwmp::ActorList& actorList);
        void readStatsDynamic(mwmp::ActorList& actorList);
        void readSpeech(mwmp::ActorList& actorList);
        void readAttack(mwmp::ActorList& actorList);
        void readCellChange(mwmp::ActorList& actorList);

        void setLocalActorRecord(std::string actorIndex, std::string cellIndex);
        void removeLocalActorRecord(std::string actorIndex);
        
        bool isLocalActor(MWWorld::Ptr ptr);
        bool isLocalActor(std::string refId, int refNumIndex, int mpNum);
        virtual LocalActor *getLocalActor(MWWorld::Ptr ptr);
        virtual LocalActor *getLocalActor(std::string refId, int refNumIndex, int mpNum);

        void setDedicatedActorRecord(std::string actorIndex, std::string cellIndex);
        void removeDedicatedActorRecord(std::string actorIndex);
        
        bool isDedicatedActor(MWWorld::Ptr ptr);
        bool isDedicatedActor(std::string refId, int refNumIndex, int mpNum);
        virtual DedicatedActor *getDedicatedActor(MWWorld::Ptr ptr);
        virtual DedicatedActor *getDedicatedActor(std::string refId, int refNumIndex, int mpNum);

        std::string generateMapIndex(std::string refId, int refNumindex, int mpNum);
        std::string generateMapIndex(MWWorld::Ptr ptr);
        std::string generateMapIndex(mwmp::BaseActor baseActor);

        bool isActiveCell(const ESM::Cell& cell);
        virtual Cell *getCell(const ESM::Cell& cell);

        virtual MWWorld::CellStore *getCellStore(const ESM::Cell& cell);

        bool isSameCell(const ESM::Cell& cell, const ESM::Cell& otherCell);

        void openContainer(const MWWorld::Ptr& container,  bool loot);
        void closeContainer(const MWWorld::Ptr& container);

        int getCellSize() const;

    private:
        static std::map<std::string, mwmp::Cell *> cellsActive;
        static std::map<std::string, std::string> localActorsToCells;
        static std::map<std::string, std::string> dedicatedActorsToCells;
    };
}

#endif //OPENMW_CELLCONTROLLER_HPP
