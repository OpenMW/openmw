//
// Created by koncord on 16.04.17.
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

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (!isRequest())
                {
                    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_PLAYER_POSITION changed by server");

                    static_cast<LocalPlayer*>(player)->setPosition();
                }
                else
                    static_cast<LocalPlayer*>(player)->updatePosition(true);
            }
            else if (player != 0) // dedicated player
                static_cast<DedicatedPlayer*>(player)->updateMarker();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERPOSITION_HPP
