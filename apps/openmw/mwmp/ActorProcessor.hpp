//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_ACTORPROCESSOR_HPP
#define OPENMW_ACTORPROCESSOR_HPP

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include "WorldEvent.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"
#include "ActorList.hpp"

namespace mwmp
{
    class ActorProcessor : public BasePacketProcessor
    {
    public:
        virtual void Do(ActorPacket &packet, ActorList &actorList) = 0;

        static bool Process(RakNet::Packet &packet, ActorList &actorList);
        static void AddProcessor(ActorProcessor *processor);
        static void SetServerAddr(RakNet::SystemAddress addr)
        {
            serverAddr = addr;
        }

        typedef boost::unordered_map<unsigned char, boost::shared_ptr<ActorProcessor> > processors_t;

    protected:
        inline bool isRequest()
        {
            return request;
        }
        LocalPlayer *getLocalPlayer();
    protected:
        static RakNet::RakNetGUID guid;
        static RakNet::SystemAddress serverAddr;
    private:
        static processors_t processors;
        static bool request;
    };
}


#endif //OPENMW_ACTORPROCESSOR_HPP
