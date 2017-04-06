#ifndef OPENMW_PROCESSORACTORFRAME_HPP
#define OPENMW_PROCESSORACTORFRAME_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorActorFrame : public WorldProcessor
    {
    public:
        ProcessorActorFrame()
        {
            BPP_INIT(ID_ACTOR_FRAME)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&event.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &event);

            Script::Call<Script::CallbackIdentity("OnActorFrame")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORFRAME_HPP
