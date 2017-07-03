#ifndef OPENMW_PROCESSORACTORDEATH_HPP
#define OPENMW_PROCESSORACTORDEATH_HPP

#include "../ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorDeath : public ActorProcessor
    {
    public:
        ProcessorActorDeath()
        {
            BPP_INIT(ID_ACTOR_DEATH)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr && *serverCell->getAuthority() == actorList.guid)
                serverCell->sendToLoaded(&packet, &actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORDEATH_HPP
