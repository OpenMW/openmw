#include "ActorProcessor.hpp"
#include "Networking.hpp"

using namespace mwmp;

ActorProcessor::processors_t ActorProcessor::processors;

void ActorProcessor::Do(ActorPacket &packet, Player &player, BaseActorList &actorList)
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

bool ActorProcessor::Process(RakNet::Packet &packet, BaseActorList &actorList) noexcept
{
    // Clear our BaseActorList before loading new data in it
    actorList.cell.blank();
    actorList.baseActors.clear();
    actorList.guid = packet.guid;

    for (auto &processor : processors)
    {
        if (processor.first == packet.data[0])
        {
            Player *player = Players::getPlayer(packet.guid);
            ActorPacket *myPacket = Networking::get().getActorPacketController()->GetPacket(packet.data[0]);

            myPacket->setActorList(&actorList);
            actorList.isValid = true;

            if (!processor.second->avoidReading)
                myPacket->Read();

            if (actorList.isValid)
                processor.second->Do(*myPacket, *player, actorList);
            else
                LOG_MESSAGE_SIMPLE(Log::LOG_ERROR, "Received %s that failed integrity check and was ignored!", processor.second->strPacketID.c_str());

            return true;
        }
    }
    return false;
}
