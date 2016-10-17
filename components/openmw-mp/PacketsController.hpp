//
// Created by koncord on 15.01.16.
//

#ifndef OPENMW_PACKETSCONTROLLER_HPP
#define OPENMW_PACKETSCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "Packets/PlayerPacket.hpp"
#include <map>
#include <boost/shared_ptr.hpp>

namespace mwmp
{
    class PacketsController
    {
    public:
        PacketsController(RakNet::RakPeerInterface *peer);
        PlayerPacket *GetPacket(RakNet::MessageID id);
        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

        typedef std::map<unsigned char, boost::shared_ptr<PlayerPacket> > packets_t;
    private:
        packets_t packets;
    };
}

#endif //OPENMW_PACKETSCONTROLLER_HPP
