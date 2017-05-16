//
// Created by koncord on 16.04.17.
//

#include <boost/foreach.hpp>
#include "WorldProcessor.hpp"
#include "Main.hpp"
#include "Networking.hpp"

using namespace mwmp;

WorldProcessor::processors_t WorldProcessor::processors;

bool WorldProcessor::Process(RakNet::Packet &packet, WorldEvent &event)
{
    RakNet::BitStream bsIn(&packet.data[1], packet.length, false);
    bsIn.Read(guid);

    WorldPacket *myPacket = Main::get().getNetworking()->getWorldPacket(packet.data[0]);

    myPacket->setEvent(&event);
    myPacket->SetReadStream(&bsIn);

    BOOST_FOREACH(processors_t::value_type &processor, processors)
    {
        if (processor.first == packet.data[0])
        {
            myGuid = Main::get().getLocalPlayer()->guid;
            request = packet.length == myPacket->headerSize();

            event.isValid = true;

            if (!request && !processor.second->avoidReading)
            {
                myPacket->Read();
            }

            if (event.isValid)
                processor.second->Do(*myPacket, event);
            else
                LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received %s that failed integrity check and was ignored!", processor.second->strPacketID.c_str());

            return true;
        }
    }
    return false;
}

void WorldProcessor::AddProcessor(mwmp::WorldProcessor *processor)
{
    BOOST_FOREACH(processors_t::value_type &p, processors)
    {
        if (processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), boost::shared_ptr<WorldProcessor>(processor)));
}
