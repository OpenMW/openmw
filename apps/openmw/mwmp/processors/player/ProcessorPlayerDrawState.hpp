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
            BPP_INIT(ID_PLAYER_DRAWSTATE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if(isRequest())
                    static_cast<LocalPlayer *>(player)->updateDrawStateAndFlags(true);
            }
            else
                static_cast<DedicatedPlayer *>(player)->updateDrawState();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERDRAWSTATE_HPP
