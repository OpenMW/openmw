//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_WORLDPROCESSSOR_HPP
#define OPENMW_WORLDPROCESSSOR_HPP

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/World/WorldPacket.hpp>
#include <unordered_map>
#include "../WorldEvent.hpp"
#include "../LocalPlayer.hpp"
#include "../DedicatedPlayer.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class WorldProcessor : public BaseClientPacketProcessor
    {
    public:
        virtual void Do(WorldPacket &packet, WorldEvent &event) = 0;

        static bool Process(RakNet::Packet &packet, WorldEvent &event);
        static void AddProcessor(WorldProcessor *processor);

        typedef std::unordered_map<unsigned char, std::unique_ptr<WorldProcessor> > processors_t;

    private:
        static processors_t processors;
    };
}


#endif //OPENMW_WORLDPROCESSSOR_HPP
