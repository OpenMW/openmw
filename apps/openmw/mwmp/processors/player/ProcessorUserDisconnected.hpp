//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORUSERDISCONNECTED_HPP
#define OPENMW_PROCESSORUSERDISCONNECTED_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"
#include "apps/openmw/mwstate/statemanagerimp.hpp"

namespace mwmp
{
    class ProcessorUserDisconnected : public PlayerProcessor
    {
    public:
        ProcessorUserDisconnected()
        {
            BPP_INIT(ID_USER_DISCONNECTED)
            avoidReading = true;
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
                MWBase::Environment::get().getStateManager()->requestQuit();
            else
                Players::disconnectPlayer(guid);
        }
    };
}

#endif //OPENMW_PROCESSORUSERDISCONNECTED_HPP
