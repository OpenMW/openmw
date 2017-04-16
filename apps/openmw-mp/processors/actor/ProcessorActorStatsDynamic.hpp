#ifndef OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP
#define OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorStatsDynamic : public ActorProcessor
    {
    public:
        ProcessorActorStatsDynamic()
        {
            BPP_INIT(ID_ACTOR_STATS_DYNAMIC)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorStatsDynamic")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP
