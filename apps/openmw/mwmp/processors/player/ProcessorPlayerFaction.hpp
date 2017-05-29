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
            if (isRequest())
            {
                // Entire faction membership cannot currently be requested from players
            }
            else if (player != 0)
            {
                static_cast<LocalPlayer*>(player)->setFactions();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERFACTION_HPP
