#ifndef OPENMW_PROCESSORPLAYERJAIL_HPP
#define OPENMW_PROCESSORPLAYERJAIL_HPP

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwgui/windowmanagerimp.hpp"

#include "../PlayerProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/Networking.hpp"

namespace mwmp
{
    class ProcessorPlayerJail : public PlayerProcessor
    {
    public:
        ProcessorPlayerJail()
        {
            BPP_INIT(ID_PLAYER_JAIL)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_JAIL from server");
            
            if (isLocal())
            {
                // Apply death penalties
                if (player->jailDays > 0)
                {
                    MWBase::Environment::get().getWindowManager()->goToJail(player->jailDays);
                }
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERJAIL_HPP
