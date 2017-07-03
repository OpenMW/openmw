#ifndef OPENMW_PROCESSORPLAYERMAP_HPP
#define OPENMW_PROCESSORPLAYERMAP_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerMap : public PlayerProcessor
    {
    public:
        ProcessorPlayerMap()
        {
            BPP_INIT(ID_PLAYER_MAP)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                static_cast<LocalPlayer*>(player)->setMapExplored();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERMAP_HPP
