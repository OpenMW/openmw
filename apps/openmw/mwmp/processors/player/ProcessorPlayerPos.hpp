//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERPOS_HPP
#define OPENMW_PROCESSORPLAYERPOS_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerPos : public PlayerProcessor
    {
    public:
        ProcessorPlayerPos()
        {
            BPP_INIT(ID_PLAYER_POS)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (!isRequest())
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_PLAYER_POS changed by server");

                    static_cast<LocalPlayer*>(player)->setPosition();
                }
                else
                    static_cast<LocalPlayer*>(player)->updatePosition(true);
            }
            else // dedicated player
                static_cast<DedicatedPlayer*>(player)->updateMarker();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERPOS_HPP
