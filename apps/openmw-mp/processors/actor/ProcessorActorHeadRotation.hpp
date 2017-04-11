#ifndef OPENMW_PROCESSORACTORHEADROTATION_HPP
#define OPENMW_PROCESSORACTORHEADROTATION_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorHeadRotation : public ActorProcessor
    {
    public:
        ProcessorActorHeadRotation()
        {
            BPP_INIT(ID_ACTOR_HEAD_ROTATION)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorHeadRotation")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORHEADROTATION_HPP
