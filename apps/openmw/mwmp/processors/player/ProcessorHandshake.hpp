//
// Created by koncord on 04.04.17.
//

#ifndef OPENMW_PROCESSORHANDSHAKE_HPP
#define OPENMW_PROCESSORHANDSHAKE_HPP

#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorHandshake : public PlayerProcessor
    {
    public:
        ProcessorHandshake()
        {
            BPP_INIT(ID_HANDSHAKE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            packet.setPlayer(getLocalPlayer()); // player is 0 because myGuid will be setted after ProcessUserMyID
            packet.Send(serverAddr);
        }
    };
}

#endif //OPENMW_PROCESSORHANDSHAKE_HPP
