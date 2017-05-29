#ifndef OPENMW_PROCESSORACTORPOSITION_HPP
#define OPENMW_PROCESSORACTORPOSITION_HPP

#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorPosition : public ActorProcessor
    {
    public:
        ProcessorActorPosition()
        {
            BPP_INIT(ID_ACTOR_POSITION);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readPositions(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORPOSITION_HPP
