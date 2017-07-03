//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
#define OPENMW_PROCESSORPLAYERANIMFLAGS_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerAnimFlags : public PlayerProcessor
    {
    public:
        ProcessorPlayerAnimFlags()
        {
            BPP_INIT(ID_PLAYER_ANIM_FLAGS)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            player.sendToLoaded(&packet);
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
