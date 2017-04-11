#ifndef OPENMW_PROCESSORACTORSPEECH_HPP
#define OPENMW_PROCESSORACTORSPEECH_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorSpeech : public ActorProcessor
    {
    public:
        ProcessorActorSpeech()
        {
            BPP_INIT(ID_ACTOR_SPEECH)
        }

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&actorList.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &actorList);

            //Script::Call<Script::CallbackIdentity("OnActorSpeech")>(player.getId(), actorList.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORSPEECH_HPP
