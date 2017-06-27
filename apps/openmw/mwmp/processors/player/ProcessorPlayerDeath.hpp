//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERDEATH_HPP
#define OPENMW_PROCESSORPLAYERDEATH_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDeath : public PlayerProcessor
    {
    public:
        ProcessorPlayerDeath()
        {
            BPP_INIT(ID_PLAYER_DEATH)
            avoidReading = true;
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_DEATH from server");
            if (isLocal())
            {
                LOG_APPEND(Log::LOG_INFO, "- Packet was about me");

                player->creatureStats.mDead = true;

                MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWMechanics::DynamicStat<float> health = playerPtr.getClass().getCreatureStats(playerPtr).getHealth();
                health.setCurrent(0);
                playerPtr.getClass().getCreatureStats(playerPtr).setHealth(health);
                packet.setPlayer(player);
                packet.Send(serverAddr);
            }
            else if (player != 0)
            {
                LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", player->npc.mName.c_str());

                MWMechanics::DynamicStat<float> health;
                player->creatureStats.mDead = true;
                health.readState(player->creatureStats.mDynamic[0]);
                health.setCurrent(0);
                health.writeState(player->creatureStats.mDynamic[0]);

                MWWorld::Ptr ptr = static_cast<DedicatedPlayer*>(player)->getPtr();
                ptr.getClass().getCreatureStats(ptr).setHealth(health);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDEATH_HPP
