#ifndef OPENMW_PROCESSORACTORANIMFLAGS_HPP
#define OPENMW_PROCESSORACTORANIMFLAGS_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorAnimFlags : public ActorProcessor
    {
    public:
        ProcessorActorAnimFlags()
        {
            BPP_INIT(ID_ACTOR_ANIM_FLAGS)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorAnimFlags")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORANIMFLAGS_HPP
