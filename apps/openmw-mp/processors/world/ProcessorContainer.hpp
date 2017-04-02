//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSORCONTAINER_HPP
#define OPENMW_PROCESSORCONTAINER_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorContainer : public WorldProcessor
    {
    public:
        ProcessorContainer()
        {
            BPP_INIT(ID_CONTAINER)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            LOG_APPEND(Log::LOG_WARN, "- action: %i", event.action);

            // Until we have a timestamp-based system, send packets pertaining to more
            // than one container (i.e. replies to server requests for container contents)
            // only to players who have the container's cell loaded
            if (event.action == BaseEvent::SET && event.objectChanges.count > 1)
                CellController::get()->getCell(&event.cell)->sendToLoaded(&packet, &event);
            else
                packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnContainer")>(player.getId(), event.cell.getDescription().c_str());

            LOG_APPEND(Log::LOG_INFO, "- Finished processing ID_CONTAINER");
        }
    };
}

#endif //OPENMW_PROCESSORCONTAINER_HPP
