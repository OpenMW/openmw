//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORUSERMYID_HPP
#define OPENMW_PROCESSORUSERMYID_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorUserMyID : public PlayerProcessor
    {
    public:
        ProcessorUserMyID()
        {
            BPP_INIT(ID_USER_MYID)
            avoidReading = true;
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_USER_MYID from server");
            myGuid = guid;
            getLocalPlayer()->guid = guid;
        }
    };
}

#endif //OPENMW_PROCESSORUSERMYID_HPP
