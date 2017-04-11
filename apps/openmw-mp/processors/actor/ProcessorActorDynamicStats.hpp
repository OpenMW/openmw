#ifndef OPENMW_PROCESSORACTORDYNAMICSTATS_HPP
#define OPENMW_PROCESSORACTORDYNAMICSTATS_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorDynamicStats : public ActorProcessor
    {
    public:
        ProcessorActorDynamicStats()
        {
            BPP_INIT(ID_ACTOR_DYNAMICSTATS)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorDynamicStats")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORDYNAMICSTATS_HPP
