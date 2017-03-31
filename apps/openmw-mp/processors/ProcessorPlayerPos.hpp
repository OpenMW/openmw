//
// Created by koncord on 31.03.17.
//

#ifndef OPENMW_PROCESSORPLAYERPOS_HPP
#define OPENMW_PROCESSORPLAYERPOS_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerPos : public PlayerProcessor
    {
    public:
        ProcessorPlayerPos()
        {
            BPP_INIT(ID_PLAYER_POS)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            //DEBUG_PRINTF(strPacketID);
            if (!player.creatureStats.mDead)
            {
                packet.setPlayer(&player);
                packet.Read();
                //myPacket.Send(player, true); //send to other clients

                player.sendToLoaded(&packet);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERPOS_HPP
