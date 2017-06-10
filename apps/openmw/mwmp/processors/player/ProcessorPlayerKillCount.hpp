#ifndef OPENMW_PROCESSORPLAYERKILLCOUNT_HPP
#define OPENMW_PROCESSORPLAYERKILLCOUNT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerKillCount : public PlayerProcessor
    {
    public:
        ProcessorPlayerKillCount()
        {
            BPP_INIT(ID_PLAYER_KILL_COUNT)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isRequest())
            {
                // Entire list of topics cannot currently be requested from players
            }
            else if (player != 0)
            {
                static_cast<LocalPlayer*>(player)->setKills();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERKILLCOUNT_HPP
