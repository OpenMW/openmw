#ifndef OPENMW_PROCESSORACTORFRAME_HPP
#define OPENMW_PROCESSORACTORFRAME_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorFrame : public ActorProcessor
    {
    public:
        ProcessorActorFrame()
        {
            BPP_INIT(ID_ACTOR_FRAME)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            Script::Call<Script::CallbackIdentity("OnActorFrame")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORFRAME_HPP
