#include "ActorProcessor.hpp"
#include "Networking.hpp"

using namespace mwmp;

ActorProcessor::processors_t ActorProcessor::processors;

void ActorProcessor::Do(ActorPacket &packet, Player &player, BaseEvent &event)
{
    packet.Send(true);
}

void ActorProcessor::AddProcessor(mwmp::ActorProcessor *processor) noexcept
{
    for (auto &p : processors)
    {
        if (processor->packetID == p.first)
            throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                   processor->className + " and " + p.second->className);
    }
    processors.insert(processors_t::value_type(processor->GetPacketID(), processor));
}

bool ActorProcessor::Process(RakNet::Packet &packet, BaseEvent &event) noexcept
{
    // Clear our BaseEvent before loading new data in it
    event.cell.blank();
    event.objectChanges.objects.clear();
    event.guid = packet.guid;
    for (auto &processor : processors)
    {
        if (processor.first == packet.data[0])
        {
            Player *player = Players::getPlayer(packet.guid);
            ActorPacket *myPacket = Networking::get().getActorPacketController()->GetPacket(packet.data[0]);

            myPacket->setEvent(&event);

            if (!processor.second->avoidReading)
                myPacket->Read();

            processor.second->Do(*myPacket, *player, event);
            return true;
        }
    }
    return false;
}
