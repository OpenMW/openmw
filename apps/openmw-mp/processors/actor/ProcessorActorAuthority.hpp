#ifndef OPENMW_PROCESSORACTORAUTHORITY_HPP
#define OPENMW_PROCESSORACTORAUTHORITY_HPP

#include "apps/openmw-mp/ActorProcessor.hpp"

namespace mwmp
{
    class ProcessorActorAuthority : public ActorProcessor
    {
    public:
        ProcessorActorAuthority()
        {
            BPP_INIT(ID_ACTOR_AUTHORITY)
        }

        void Do(ActorPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);
        }
    };
}

#endif //OPENMW_PROCESSORACTORAUTHORITY_HPP
