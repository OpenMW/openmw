//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_ACTORPROCESSOR_HPP
#define OPENMW_ACTORPROCESSOR_HPP

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include "../WorldEvent.hpp"
#include "../ActorList.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class ActorProcessor : public BasePacketProcessor<ActorProcessor>, public BaseClientPacketProcessor
    {
    public:
        virtual void Do(ActorPacket &packet, ActorList &actorList) = 0;

        static bool Process(RakNet::Packet &packet, ActorList &actorList);
    };
}


#endif //OPENMW_ACTORPROCESSOR_HPP
