#ifndef OPENMW_PROCESSORACTORDRAWSTATE_HPP
#define OPENMW_PROCESSORACTORDRAWSTATE_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorDrawState : public ActorProcessor
    {
    public:
        ProcessorActorDrawState()
        {
            BPP_INIT(ID_ACTOR_DRAW_STATE)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorDrawState")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORDRAWSTATE_HPP
