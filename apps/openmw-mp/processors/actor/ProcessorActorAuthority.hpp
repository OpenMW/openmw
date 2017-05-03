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

        void Do(ActorPacket &packet, Player &player, BaseActorList &actorList) override
        {
            // In the current implementation, only the server should be able to send ActorAuthority packets
        }
    };
}

#endif //OPENMW_PROCESSORACTORAUTHORITY_HPP
