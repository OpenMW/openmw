#ifndef OPENMW_PROCESSORPLAYERFACTION_HPP
#define OPENMW_PROCESSORPLAYERFACTION_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerFaction : public PlayerProcessor
    {
    public:
        ProcessorPlayerFaction()
        {
            BPP_INIT(ID_PLAYER_FACTION)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (!isLocal()) return;
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERFACTION_HPP
