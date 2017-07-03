#ifndef OPENMW_PROCESSORACTORPOSITION_HPP
#define OPENMW_PROCESSORACTORPOSITION_HPP

#include "../ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorPosition : public ActorProcessor
    {
    public:
        ProcessorActorPosition()
        {
            BPP_INIT(ID_ACTOR_POSITION)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr && *serverCell->getAuthority() == actorList.guid)
            {
                serverCell->readActorList(packetID, &actorList);
                serverCell->sendToLoaded(&packet, &actorList);
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORPOSITION_HPP
