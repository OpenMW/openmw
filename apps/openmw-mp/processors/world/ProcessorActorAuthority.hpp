#ifndef OPENMW_PROCESSORACTORAUTHORITY_HPP
#define OPENMW_PROCESSORACTORAUTHORITY_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorActorAuthority : public WorldProcessor
    {
    public:
        ProcessorActorAuthority()
        {
            BPP_INIT(ID_ACTOR_AUTHORITY)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);
        }
    };
}

#endif //OPENMW_PROCESSORACTORAUTHORITY_HPP
