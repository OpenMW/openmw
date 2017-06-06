//
// Created by koncord on 04.04.17.
//

#include <boost/foreach.hpp>
#include "../Networking.hpp"
#include "PlayerProcessor.hpp"
#include "../Main.hpp"

using namespace mwmp;

PlayerProcessor::processors_t PlayerProcessor::processors;

void PlayerProcessor::AddProcessor(PlayerProcessor *processor)
{
    BOOST_FOREACH(processors_t::value_type &p, processors)
    {
        if (processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), boost::shared_ptr<PlayerProcessor>(processor)));
}

bool PlayerProcessor::Process(RakNet::Packet &packet)
{
    RakNet::BitStream bsIn(&packet.data[1], packet.length, false);
    bsIn.Read(guid);

    PlayerPacket *myPacket = Main::get().getNetworking()->getPlayerPacket(packet.data[0]);
    myPacket->SetReadStream(&bsIn);

    /*if (myPacket == 0)
    {
        // error: packet not found
    }*/

    BOOST_FOREACH(processors_t::value_type &processor, processors)
    {
        if (processor.first == packet.data[0])
        {
            myGuid = Main::get().getLocalPlayer()->guid;
            request = packet.length == myPacket->headerSize();

            BasePlayer *player = 0;
            if (guid != myGuid)
                player = PlayerList::getPlayer(guid);
            else
                player = Main::get().getLocalPlayer();

            if (!request && !processor.second->avoidReading && player != 0)
            {
                myPacket->setPlayer(player);
                myPacket->Read();
            }

            processor.second->Do(*myPacket, player);
            return true;
        }
    }
    return false;
}
