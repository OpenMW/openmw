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

            Player *killer = Players::getPlayer(player.getLastAttackerId());

            short reason = 0; // unknown;
            double secondsSinceLastAttacker = std::chrono::duration_cast<std::chrono::duration<double>>(
                    std::chrono::steady_clock::now() - player.getLastAttackerTime()).count();

            if (!killer)
                killer = &player;

            if (secondsSinceLastAttacker < 3.0f)
                reason = 1; // killed
            else
                reason = 2; //suicide

            player.resetLastAttacker();

            player.creatureStats.mDead = true;

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerDeath")>(player.getId(), reason, killer->getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDEATH_HPP
