//
// Created by koncord on 03.04.17.
//

#include "WorldProcessor.hpp"
#include "Networking.hpp"

using namespace mwmp;

WorldProcessor::processors_t WorldProcessor::processors;

void WorldProcessor::AddProcessor(mwmp::WorldProcessor *processor) noexcept
{
    for(auto &p : processors)
    {
        if(processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), processor));
}

bool WorldProcessor::Process(RakNet::Packet &packet) noexcept
{
    BaseEvent baseEvent;
    baseEvent.cell.blank();
    baseEvent.objectChanges.objects.clear();
    baseEvent.guid = packet.guid;
    for(auto &processor : processors)
    {
        if(processor.first == packet.data[0])
        {
            Player *player = Players::getPlayer(packet.guid);
            WorldPacket *myPacket = Networking::get().getWorldController()->GetPacket(packet.data[0]);

            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s from %s", processor.second->strPacketID.c_str(),
                               player->npc.mName.c_str());
            myPacket->setEvent(&baseEvent);
            myPacket->Read();
            processor.second->Do(*myPacket, *player, baseEvent);
            return true;
        }
    }
    return false;
}
