//
// Created by koncord on 31.03.17.
//

#ifndef OPENMW_BASEPLAYERPROCESSOR_HPP
#define OPENMW_BASEPLAYERPROCESSOR_HPP

#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <unordered_map>
#include <memory>
//#include <boost/unordered_map.hpp>
//#include <boost/shared_ptr.hpp>
#include "Player.hpp"

namespace mwmp
{
    class PlayerProcessor : public BasePacketProcessor
    {
    public:

        virtual void Do(PlayerPacket &packet, Player &player) = 0;

        static bool Process(RakNet::Packet &packet) noexcept;
        static void AddProcessor(PlayerProcessor *processor) noexcept;

        //typedef boost::unordered_map<unsigned char, boost::shared_ptr<PlayerProcessor> > processors_t;
        typedef std::unordered_map<unsigned char, std::unique_ptr<PlayerProcessor> > processors_t;
    private:
        static processors_t processors;
    };
}

#endif //OPENMW_BASEPLAYERPROCESSOR_HPP
