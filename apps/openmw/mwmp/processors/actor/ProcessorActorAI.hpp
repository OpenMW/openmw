#ifndef OPENMW_PROCESSORACTORAI_HPP
#define OPENMW_PROCESSORACTORAI_HPP

#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorAI : public ActorProcessor
    {
    public:
        ProcessorActorAI()
        {
            BPP_INIT(ID_ACTOR_AI);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            //Main::get().getCellController()->readAI(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORAI_HPP
