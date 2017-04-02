//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERRESURRECT_HPP
#define OPENMW_PROCESSORPLAYERRESURRECT_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerResurrect : public PlayerProcessor
    {
    public:
        ProcessorPlayerResurrect()
        {
            BPP_INIT(ID_PLAYER_RESURRECT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            player.creatureStats.mDead = false;

            packet.setPlayer(&player);
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerResurrect")>(player.getId());
        }
    };
}



#endif //OPENMW_PROCESSORPLAYERRESURRECT_HPP
