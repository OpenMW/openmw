//
// Created by koncord on 31.03.17.
//

#ifndef OPENMW_PROCESSORPLAYERPOSITION_HPP
#define OPENMW_PROCESSORPLAYERPOSITION_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerPosition : public PlayerProcessor
    {
    public:
        ProcessorPlayerPosition()
        {
            BPP_INIT(ID_PLAYER_POSITION)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            //DEBUG_PRINTF(strPacketID);
            if (!player.creatureStats.mDead)
            {
                player.sendToLoaded(&packet);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERPOSITION_HPP
