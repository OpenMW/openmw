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
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s about %s", strPacketID.c_str(), actorList.cell.getDescription().c_str());

            // Never initialize LocalActors in a cell that is no longer loaded, if the server's packet arrived too late
            if (mwmp::Main::get().getCellController()->isActiveWorldCell(actorList.cell))
            {
                if (isLocal())
                {
                    LOG_APPEND(Log::LOG_INFO, "- The new authority is me");
                    Main::get().getCellController()->initializeLocalActors(actorList.cell);
                    Main::get().getCellController()->getCell(actorList.cell)->updateLocal(true);
                }
                else
                {
                    BasePlayer *player = PlayerList::getPlayer(guid);

                    if (player != 0)
                        LOG_APPEND(Log::LOG_INFO, "- The new authority is %s", player->npc.mName.c_str());

                    Main::get().getCellController()->getCell(actorList.cell)->uninitializeLocalActors();
                }
            }
            else
            {
                LOG_APPEND(Log::LOG_INFO, "- Ignoring it because that cell isn't loaded");
            }
        }
    };
}

#endif //OPENMW_PROCESSORACTORAUTHORITY_HPP
