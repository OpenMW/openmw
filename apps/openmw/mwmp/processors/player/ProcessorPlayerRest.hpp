#ifndef OPENMW_PROCESSORPLAYERREST_HPP
#define OPENMW_PROCESSORPLAYERREST_HPP

#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerRest : public PlayerProcessor
    {
    public:
        ProcessorPlayerRest()
        {
            BPP_INIT(ID_PLAYER_REST)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERREST_HPP
