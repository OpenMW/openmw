//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PLAYERPROCESSOR_HPP
#define OPENMW_PLAYERPROCESSOR_HPP

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"

namespace mwmp
{
    class PlayerProcessor : public BasePacketProcessor
    {
    public:
        virtual void Do(PlayerPacket &packet, BasePlayer *player) = 0;

        static bool Process(RakNet::Packet &packet);
        static void AddProcessor(PlayerProcessor *processor);
        static void SetServerAddr(RakNet::SystemAddress addr)
        {
            serverAddr = addr;
        }

        typedef boost::unordered_map<unsigned char, boost::shared_ptr<PlayerProcessor> > processors_t;
        //typedef std::unordered_map<unsigned char, std::unique_ptr<PlayerProcessor> > processors_t;
    private:
        static processors_t processors;
        static bool request;
    protected:
        inline bool isRequest()
        {
            return request;
        }

        inline bool isLocal()
        {
            return guid == myGuid;
        }

        LocalPlayer *getLocalPlayer();
    protected:
        static RakNet::RakNetGUID myGuid;
        static RakNet::RakNetGUID guid;
        static RakNet::SystemAddress serverAddr;
    };
}



#endif //OPENMW_PLAYERPROCESSOR_HPP
