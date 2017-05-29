#ifndef OPENMW_PROCESSORPLAYERTOPIC_HPP
#define OPENMW_PROCESSORPLAYERTOPIC_HPP

#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerTopic : public PlayerProcessor
    {
    public:
        ProcessorPlayerTopic()
        {
            BPP_INIT(ID_PLAYER_TOPIC)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isRequest())
            {
                // Entire list of topics cannot currently be requested from players
            }
            else if (player != 0)
            {
                static_cast<LocalPlayer*>(player)->addTopics();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERTOPIC_HPP
