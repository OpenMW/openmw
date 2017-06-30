#ifndef OPENMW_ACTORPROCESSOR_HPP
#define OPENMW_ACTORPROCESSOR_HPP


#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <unordered_map>
#include <memory>
#include "Script/Script.hpp"
#include "Player.hpp"

namespace mwmp
{
    class ActorProcessor : public BasePacketProcessor
    {
    public:

        virtual void Do(ActorPacket &packet, Player &player, BaseActorList &actorList);

        static bool Process(RakNet::Packet &packet, BaseActorList &actorList) noexcept;
        static void AddProcessor(ActorProcessor *processor) noexcept;

        typedef std::unordered_map<unsigned char, std::unique_ptr<ActorProcessor> > processors_t;
    private:
        static processors_t processors;
    };
}

#endif //OPENMW_ACTORPROCESSOR_HPP
