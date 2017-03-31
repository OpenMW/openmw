//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERATTACK_HPP
#define OPENMW_PROCESSORPLAYERATTACK_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerAttack : public PlayerProcessor
    {
        PlayerPacketController *playerController;
    public:
        ProcessorPlayerAttack()
        {
            BPP_INIT(ID_PLAYER_ATTACK)
            playerController = Networking::get().getPlayerController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            if (!player.creatureStats.mDead)
            {

                packet.setPlayer(&player);
                packet.Read();

                Player *target = Players::getPlayer(player.attack.target);

                if (target == nullptr)
                    target = &player;

                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Player: %s attacked %s state: %d", player.npc.mName.c_str(),
                                   target->npc.mName.c_str(), player.attack.pressed == 1);
                if (player.attack.pressed == 0)
                {
                    LOG_APPEND(Log::LOG_VERBOSE, "success: %d", player.attack.success == 1);
                    if (player.attack.success == 1)
                    {
                        LOG_APPEND(Log::LOG_VERBOSE, "damage: %d", player.attack.damage == 1);
                        target->setLastAttackerId(player.getId());
                        target->setLastAttackerTime(std::chrono::steady_clock::now());
                    }
                }

                //packet.Send(player, true);
                player.sendToLoaded(&packet);
                playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->RequestData(player.attack.target);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERATTACK_HPP
