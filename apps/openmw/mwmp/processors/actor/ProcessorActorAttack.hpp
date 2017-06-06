#ifndef OPENMW_PROCESSORACTORATTACK_HPP
#define OPENMW_PROCESSORACTORATTACK_HPP

#include "../ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorAttack : public ActorProcessor
    {
    public:
        ProcessorActorAttack()
        {
            BPP_INIT(ID_ACTOR_ATTACK);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readAttack(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORATTACK_HPP
