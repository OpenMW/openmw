//
// Created by koncord on 18.04.17.
//

#include "ActorProcessor.hpp"
#include "../Networking.hpp"
#include "../Main.hpp"

using namespace mwmp;

ActorProcessor::processors_t ActorProcessor::processors;

bool ActorProcessor::Process(RakNet::Packet &packet, ActorList &actorList)
{
    RakNet::BitStream bsIn(&packet.data[1], packet.length, false);
    bsIn.Read(guid);

    ActorPacket *myPacket = Main::get().getNetworking()->getActorPacket(packet.data[0]);

    myPacket->setActorList(&actorList);
    myPacket->SetReadStream(&bsIn);

    for (auto &processor : processors)
    {
        if (processor.first == packet.data[0])
        {
            myGuid = Main::get().getLocalPlayer()->guid;
            request = packet.length == myPacket->headerSize();

            actorList.isValid = true;

            if (!request && !processor.second->avoidReading)
            {
                myPacket->Read();
            }

            if (actorList.isValid)
                processor.second->Do(*myPacket, actorList);
            else
                LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received %s that failed integrity check and was ignored!", processor.second->strPacketID.c_str());

            return true;
        }
    }
    return false;
}

void ActorProcessor::AddProcessor(mwmp::ActorProcessor *processor)
{
    for (auto &p : processors)
    {
        if (processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), processor));
}
