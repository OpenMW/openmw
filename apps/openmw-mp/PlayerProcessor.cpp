//
// Created by koncord on 31.03.17.
//

#include "PlayerProcessor.hpp"
#include "Networking.hpp"
//#include <boost/foreach.hpp>

using namespace mwmp;

PlayerProcessor::processors_t PlayerProcessor::processors;

void PlayerProcessor::AddProcessor(PlayerProcessor *processor) noexcept
{
    for(auto &p : processors)
    {
        if(processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                           processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), processor));
}

bool PlayerProcessor::Process(RakNet::Packet &packet) noexcept
{
    //BOOST_FOREACH(processors_t::value_type &processor, processors)
    for(auto &processor : processors)
    {
        if(processor.first == packet.data[0])
        {
            Player *player = Players::getPlayer(packet.guid);
            PlayerPacket *myPacket = Networking::get().getPlayerController()->GetPacket(packet.data[0]);
            myPacket->setPlayer(player);

            if(!processor.second->dontRead)
                myPacket->Read();

            processor.second->Do(*myPacket, *player);
            return true;
        }
    }
    return false;
}