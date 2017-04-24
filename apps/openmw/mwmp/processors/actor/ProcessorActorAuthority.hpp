//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSORACTORAUTHORITY_HPP
#define OPENMW_PROCESSORACTORAUTHORITY_HPP


#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorAuthority : public ActorProcessor
    {
    public:
        ProcessorActorAuthority()
        {
            BPP_INIT(ID_ACTOR_AUTHORITY)
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s about %s", strPacketID.c_str(), actorList.cell.getDescription().c_str());

            // Never initialize LocalActors in a cell that is no longer loaded, if the server's packet arrived too late
            if (mwmp::Main::get().getCellController()->isActiveWorldCell(actorList.cell))
            {
                Main::get().getCellController()->initializeLocalActors(actorList.cell);
                Main::get().getCellController()->getCell(actorList.cell)->updateLocal(true);
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORAUTHORITY_HPP
