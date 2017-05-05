//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERDEATH_HPP
#define OPENMW_PROCESSORPLAYERDEATH_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"
#include <chrono>

namespace mwmp
{
    class ProcessorPlayerDeath : public PlayerProcessor
    {
    public:
        ProcessorPlayerDeath()
        {
            BPP_INIT(ID_PLAYER_DEATH)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerDeath")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDEATH_HPP
