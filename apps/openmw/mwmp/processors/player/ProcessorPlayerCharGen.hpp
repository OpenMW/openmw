//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERCHARGEN_HPP
#define OPENMW_PROCESSORPLAYERCHARGEN_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCharGen : public PlayerProcessor
    {
    public:
        ProcessorPlayerCharGen()
        {
            BPP_INIT(ID_PLAYER_CHARGEN)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {

        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCHARGEN_HPP
