//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_WORLDPROCESSOR_HPP
#define OPENMW_WORLDPROCESSOR_HPP


#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/Packets/World/WorldPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <unordered_map>
#include <memory>
#include "Script/Script.hpp"
#include "Player.hpp"

namespace mwmp
{
    class WorldProcessor : public BasePacketProcessor
    {
    public:

        virtual void Do(WorldPacket &packet, Player &player, BaseEvent &event);

        static bool Process(RakNet::Packet &packet, BaseEvent &event) noexcept;
        static void AddProcessor(WorldProcessor *processor) noexcept;

        typedef std::unordered_map<unsigned char, std::unique_ptr<WorldProcessor> > processors_t;
    private:
        static processors_t processors;
    };
}

#endif //OPENMW_WORLDPROCESSOR_HPP
