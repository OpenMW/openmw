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
            mwmp::CellController *cellController = Main::get().getCellController();

            // Never initialize LocalActors in a cell that is no longer loaded, if the server's packet arrived too late
            if (cellController->isActiveWorldCell(actorList.cell))
            {
                cellController->initializeCell(actorList.cell);
                mwmp::Cell *cell = cellController->getCell(actorList.cell);
                cell->setAuthority(guid);

                if (isLocal())
                {
                    LOG_APPEND(Log::LOG_INFO, "- The new authority is me");
                    cell->uninitializeDedicatedActors();
                    cell->initializeLocalActors();
                    cell->updateLocal(true);
                }
                else
                {
                    BasePlayer *player = PlayerList::getPlayer(guid);

                    if (player != 0)
                        LOG_APPEND(Log::LOG_INFO, "- The new authority is %s", player->npc.mName.c_str());

                    cell->uninitializeLocalActors();
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
