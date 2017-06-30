//
// Created by koncord on 16.04.17.
//

#include "WorldProcessor.hpp"
#include "../Main.hpp"
#include "../Networking.hpp"

using namespace mwmp;

template<class T>
typename BasePacketProcessor<T>::processors_t BasePacketProcessor<T>::processors;

bool WorldProcessor::Process(RakNet::Packet &packet, WorldEvent &event)
{
    RakNet::BitStream bsIn(&packet.data[1], packet.length, false);
    bsIn.Read(guid);

    WorldPacket *myPacket = Main::get().getNetworking()->getWorldPacket(packet.data[0]);

    myPacket->setEvent(&event);
    myPacket->SetReadStream(&bsIn);

    for (auto &processor: processors)
    {
        if (processor.first == packet.data[0])
        {
            myGuid = Main::get().getLocalPlayer()->guid;
            request = packet.length == myPacket->headerSize();

            event.isValid = true;

            if (!request && !processor.second->avoidReading)
                myPacket->Read();

            if (event.isValid)
                processor.second->Do(*myPacket, event);
            else
                LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received %s that failed integrity check and was ignored!", processor.second->strPacketID.c_str());

            return true;
        }
    }
    return false;
}
