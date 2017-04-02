//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSORDOORSTATE_HPP
#define OPENMW_PROCESSORDOORSTATE_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorDoorState : public WorldProcessor
    {
    public:
        ProcessorDoorState()
        {
            BPP_INIT(ID_DOOR_STATE)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnDoorState")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORDOORSTATE_HPP
