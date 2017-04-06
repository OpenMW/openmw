#ifndef OPENMW_PROCESSORACTORLIST_HPP
#define OPENMW_PROCESSORACTORLIST_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorActorList : public WorldProcessor
    {
    public:
        ProcessorActorList()
        {
            BPP_INIT(ID_ACTOR_LIST)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            // Send only to players who have the cell loaded
            Cell *serverCell = CellController::get()->getCell(&event.cell);

            if (serverCell != nullptr)
                serverCell->sendToLoaded(&packet, &event);

            Script::Call<Script::CallbackIdentity("OnActorList")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORLIST_HPP
