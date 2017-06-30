//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_BASECLIENTPACKETPROCESSOR_HPP
#define OPENMW_BASECLIENTPACKETPROCESSOR_HPP

#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include "../LocalPlayer.hpp"
#include "../DedicatedPlayer.hpp"

namespace mwmp
{
    class BaseClientPacketProcessor
    {
    public:
        static void SetServerAddr(RakNet::SystemAddress addr)
        {
            serverAddr = addr;
        }

    protected:
        inline bool isRequest()
        {
            return request;
        }

        inline bool isLocal()
        {
            return guid == myGuid;
        }

        LocalPlayer *getLocalPlayer();

    protected:
        static RakNet::RakNetGUID guid, myGuid;
        static RakNet::SystemAddress serverAddr;

        static bool request;
    };
}

#endif //OPENMW_BASECLIENTPACKETPROCESSOR_HPP
