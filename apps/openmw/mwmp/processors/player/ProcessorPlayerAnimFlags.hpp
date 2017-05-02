//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
#define OPENMW_PROCESSORPLAYERANIMFLAGS_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerAnimFlags : public PlayerProcessor
    {
    public:
        ProcessorPlayerAnimFlags()
        {
            BPP_INIT(ID_PLAYER_ANIM_FLAGS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateAnimFlags(true);
            }
            else if (player != 0)
                static_cast<DedicatedPlayer *>(player)->updateAnimFlags();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERANIMFLAGS_HPP
