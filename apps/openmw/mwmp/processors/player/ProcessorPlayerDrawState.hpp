//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERDRAWSTATE_HPP
#define OPENMW_PROCESSORPLAYERDRAWSTATE_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDrawState : public PlayerProcessor
    {
    public:
        ProcessorPlayerDrawState()
        {
            BPP_INIT(ID_PLAYER_ANIM_FLAGS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if(isRequest())
                    static_cast<LocalPlayer *>(player)->updateAnimFlags(true);
            }
            else
                static_cast<DedicatedPlayer *>(player)->updateAnimFlags();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERDRAWSTATE_HPP
