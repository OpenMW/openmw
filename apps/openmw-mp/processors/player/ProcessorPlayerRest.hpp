#ifndef OPENMW_PROCESSORPLAYERREST_HPP
#define OPENMW_PROCESSORPLAYERREST_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerRest : public PlayerProcessor
    {
    public:
        ProcessorPlayerRest()
        {
            BPP_INIT(ID_PLAYER_REST)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerRest")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERREST_HPP
