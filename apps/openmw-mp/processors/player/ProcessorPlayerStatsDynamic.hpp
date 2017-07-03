//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERSTATS_DYNAMIC_HPP
#define OPENMW_PROCESSORPLAYERSTATS_DYNAMIC_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerStatsDynamic : public PlayerProcessor
    {
    public:
        ProcessorPlayerStatsDynamic()
        {
            BPP_INIT(ID_PLAYER_STATS_DYNAMIC)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            player.sendToLoaded(&packet);
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSTATS_DYNAMIC_HPP
