#ifndef OPENMW_PROCESSORPLAYERREGIONAUTHORITY_HPP
#define OPENMW_PROCESSORPLAYERREGIONAUTHORITY_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerRegionAuthority : public PlayerProcessor
    {
    public:
        ProcessorPlayerRegionAuthority()
        {
            BPP_INIT(ID_PLAYER_REGION_AUTHORITY)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERREGIONAUTHORITY_HPP
