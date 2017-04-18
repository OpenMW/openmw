//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_WORLDPROCESSSOR_HPP
#define OPENMW_WORLDPROCESSSOR_HPP

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/World/WorldPacket.hpp>
#include "WorldEvent.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"

namespace mwmp
{
    class WorldProcessor : public BasePacketProcessor
    {
    public:
        virtual void Do(WorldPacket &packet, WorldEvent &event) = 0;

        static bool Process(RakNet::Packet &packet, WorldEvent &event);
        static void AddProcessor(WorldProcessor *processor);
        static void SetServerAddr(RakNet::SystemAddress addr)
        {
            serverAddr = addr;
        }

        typedef boost::unordered_map<unsigned char, boost::shared_ptr<WorldProcessor> > processors_t;

    protected:
        inline bool isRequest()
        {
            return request;
        }
        LocalPlayer *getLocalPlayer();
    protected:
        static RakNet::RakNetGUID guid;
        static RakNet::SystemAddress serverAddr;
    private:
        static processors_t processors;
        static bool request;
    };
}


#endif //OPENMW_WORLDPROCESSSOR_HPP
