#ifndef OPENMW_PROCESSORACTORCELLCHANGE_HPP
#define OPENMW_PROCESSORACTORCELLCHANGE_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorCellChange : public ActorProcessor
    {
    public:
        ProcessorActorCellChange()
        {
            BPP_INIT(ID_ACTOR_CELL_CHANGE)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
            {
                serverCell->removeActors(&actorList);

                Script::Call<Script::CallbackIdentity("OnActorCellChange")>(player.getId(), actorList.cell.getDescription().c_str());

                // Send this to everyone
                packet.Send(true);
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORCELLCHANGE_HPP
