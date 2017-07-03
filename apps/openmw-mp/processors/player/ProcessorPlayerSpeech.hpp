#ifndef OPENMW_PROCESSORPLAYERSPEECH_HPP
#define OPENMW_PROCESSORPLAYERSPEECH_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSpeech : public PlayerProcessor
    {
    public:
        ProcessorPlayerSpeech()
        {
            BPP_INIT(ID_PLAYER_SPEECH)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            player.sendToLoaded(&packet);
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSPEECH_HPP
