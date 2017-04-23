//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP
#define OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP


#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorStatsDynamic : public ActorProcessor
    {
    public:
        ProcessorActorStatsDynamic()
        {
            BPP_INIT(ID_ACTOR_STATS_DYNAMIC);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readStatsDynamic(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORSTATSDYNAMIC_HPP
