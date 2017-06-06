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

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSPEECH_HPP
