//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERDRAWSTATE_HPP
#define OPENMW_PROCESSORPLAYERDRAWSTATE_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDrawState : public PlayerProcessor
    {
    public:
        ProcessorPlayerDrawState()
        {
            BPP_INIT(ID_PLAYER_DRAWSTATE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.setPlayer(&player);
            packet.Read();
            //packet.Send(&player, true);

            player.sendToLoaded(&packet);
        }
    };
}




#endif //OPENMW_PROCESSORPLAYERDRAWSTATE_HPP
