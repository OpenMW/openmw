#ifndef OPENMW_PROCESSORPLAYERKILLCOUNT_HPP
#define OPENMW_PROCESSORPLAYERKILLCOUNT_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerKillCount : public PlayerProcessor
    {
    public:
        ProcessorPlayerKillCount()
        {
            BPP_INIT(ID_PLAYER_KILL_COUNT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerKillCount")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERKILLCOUNT_HPP
