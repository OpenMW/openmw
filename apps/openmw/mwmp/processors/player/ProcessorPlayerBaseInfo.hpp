//
// Created by koncord on 04.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERBASEINFO_HPP
#define OPENMW_PROCESSORPLAYERBASEINFO_HPP

#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerBaseInfo : public PlayerProcessor
    {
    public:
        ProcessorPlayerBaseInfo()
        {
            BPP_INIT(ID_PLAYER_BASEINFO)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_BASEINFO from server");

            if(isLocal())
            {
                LOG_APPEND(Log::LOG_INFO, "- Packet was about my id");

                if(isRequest())
                {
                    LOG_APPEND(Log::LOG_INFO, "- Requesting info");
                    packet.Send(serverAddr);
                }
                else
                {
                    LOG_APPEND(Log::LOG_INFO, "- Updating LocalPlayer");
                    static_cast<LocalPlayer*>(player)->updateChar();
                }
            }
            else
            {
                LOG_APPEND(Log::LOG_INFO, "- Packet was about %s", player == 0 ? "new player" : player->npc.mName.c_str());

                if (player == 0)
                {
                    LOG_APPEND(Log::LOG_INFO, "- Exchanging data with new player");
                    player = Players::newPlayer(guid);

                    packet.setPlayer(player);
                    packet.Read();
                }

                Players::createPlayer(guid);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERBASEINFO_HPP
