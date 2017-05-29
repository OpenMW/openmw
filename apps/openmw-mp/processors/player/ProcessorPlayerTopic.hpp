#ifndef OPENMW_PROCESSORPLAYERTOPIC_HPP
#define OPENMW_PROCESSORPLAYERTOPIC_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerTopic : public PlayerProcessor
    {
    public:
        ProcessorPlayerTopic()
        {
            BPP_INIT(ID_PLAYER_TOPIC)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerTopic")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERTOPIC_HPP
