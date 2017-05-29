#ifndef OPENMW_PROCESSORACTORCELLCHANGE_HPP
#define OPENMW_PROCESSORACTORCELLCHANGE_HPP


#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorCellChange : public ActorProcessor
    {
    public:
        ProcessorActorCellChange()
        {
            BPP_INIT(ID_ACTOR_CELL_CHANGE);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readCellChange(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORCELLCHANGE_HPP
