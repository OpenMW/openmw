#ifndef OPENMW_PROCESSORPLAYERANIMPLAY_HPP
#define OPENMW_PROCESSORPLAYERANIMPLAY_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerAnimPlay : public PlayerProcessor
    {
    public:
        ProcessorPlayerAnimPlay()
        {
            BPP_INIT(ID_PLAYER_ANIM_PLAY)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            player.sendToLoaded(&packet);
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERANIMPLAY_HPP
