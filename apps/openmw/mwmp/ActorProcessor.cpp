//
// Created by koncord on 18.04.17.
//

#include <boost/foreach.hpp>
#include "ActorProcessor.hpp"
#include "Networking.hpp"
#include "Main.hpp"

using namespace mwmp;

ActorProcessor::processors_t ActorProcessor::processors;
RakNet::RakNetGUID ActorProcessor::guid;
RakNet::SystemAddress ActorProcessor::serverAddr;
bool ActorProcessor::request;

bool ActorProcessor::Process(RakNet::Packet &packet, ActorList &actorList)
{
    RakNet::BitStream bsIn(&packet.data[1], packet.length, false);
    bsIn.Read(guid);

    ActorPacket *myPacket = Main::get().getNetworking()->getActorPacket(packet.data[0]);

    myPacket->setActorList(&actorList);
    myPacket->SetReadStream(&bsIn);

    BOOST_FOREACH(processors_t::value_type &processor, processors)
                {
                    if(processor.first == packet.data[0])
                    {
                        request = packet.length == myPacket->headerSize();

                        if(!request && !processor.second->avoidReading)
                        {
                            myPacket->Read();
                        }

                        processor.second->Do(*myPacket, actorList);

                        return true;
                    }
                }
    return false;
}

void ActorProcessor::AddProcessor(mwmp::ActorProcessor *processor)
{
    BOOST_FOREACH(processors_t::value_type &p, processors)
    {
        if(processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), boost::shared_ptr<ActorProcessor>(processor)));
}

LocalPlayer *ActorProcessor::getLocalPlayer()
{
    return Main::get().getLocalPlayer();
}
