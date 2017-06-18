//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PLAYERPROCESSOR_HPP
#define OPENMW_PLAYERPROCESSOR_HPP

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include "../LocalPlayer.hpp"
#include "../DedicatedPlayer.hpp"
#include "../PlayerList.hpp"
#include "BaseClientPacketProcessor.hpp"

namespace mwmp
{
    class PlayerProcessor : public BaseClientPacketProcessor
    {
    public:
        virtual void Do(PlayerPacket &packet, BasePlayer *player) = 0;

        static bool Process(RakNet::Packet &packet);
        static void AddProcessor(PlayerProcessor *processor);

        typedef boost::unordered_map<unsigned char, std::shared_ptr<PlayerProcessor> > processors_t;
        //typedef std::unordered_map<unsigned char, std::unique_ptr<PlayerProcessor> > processors_t;
    private:
        static processors_t processors;
    };
}



#endif //OPENMW_PLAYERPROCESSOR_HPP
