//
// Created by koncord on 03.04.17.
//

#include "WorldProcessor.hpp"
#include "Networking.hpp"

using namespace mwmp;

template<class T>
typename BasePacketProcessor<T>::processors_t BasePacketProcessor<T>::processors;

void WorldProcessor::Do(WorldPacket &packet, Player &player, BaseEvent &event)
{
    packet.Send(true);
}

bool WorldProcessor::Process(RakNet::Packet &packet, BaseEvent &event) noexcept
{
    // Clear our BaseEvent before loading new data in it
    event.cell.blank();
    event.worldObjects.clear();
    event.guid = packet.guid;

    for (auto &processor : processors)
    {
        if (processor.first == packet.data[0])
        {
            Player *player = Players::getPlayer(packet.guid);
            WorldPacket *myPacket = Networking::get().getWorldPacketController()->GetPacket(packet.data[0]);

            myPacket->setEvent(&event);
            event.isValid = true;

            if (!processor.second->avoidReading)
                myPacket->Read();

            if (event.isValid)
                processor.second->Do(*myPacket, *player, event);
            else
                LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received %s that failed integrity check and was ignored!", processor.second->strPacketID.c_str());
            
            return true;
        }
    }
    return false;
}
