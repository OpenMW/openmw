//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
#define OPENMW_PROCESSORPLAYERCELLCHANGE_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"
#include "apps/openmw-mp/Networking.hpp"
#include "apps/openmw-mp/Script/Script.hpp"
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>

namespace mwmp
{
    class ProcessorPlayerCellChange : public PlayerProcessor
    {
        PlayerPacketController *playerController;
    public:
        ProcessorPlayerCellChange()
        {
            BPP_INIT(ID_PLAYER_CELL_CHANGE)
            playerController = Networking::get().getPlayerController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            if (!player.creatureStats.mDead)
            {
                packet.Read();

                LOG_APPEND(Log::LOG_INFO, "- Moved to %s", player.cell.getDescription().c_str());

                player.forEachLoaded([this](Player *pl, Player *other) {

                    if (other == nullptr)
                    {
                        LOG_APPEND(Log::LOG_INFO, "- Tried to exchange information with nullptr!");
                        LOG_APPEND(Log::LOG_INFO, "- Please report this to a developer");
                    }
                    else
                    {
                        LOG_APPEND(Log::LOG_INFO, "- Started information exchange with %s", other->npc.mName.c_str());

                        playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->setPlayer(other);
                        playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(other);
                        playerController->GetPacket(ID_PLAYER_POS)->setPlayer(other);
                        playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(other);
                        playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(other);
                        playerController->GetPacket(ID_PLAYER_DRAWSTATE)->setPlayer(other);

                        playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(pl->guid);
                        playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(pl->guid);
                        playerController->GetPacket(ID_PLAYER_POS)->Send(pl->guid);
                        playerController->GetPacket(ID_PLAYER_SKILL)->Send(pl->guid);
                        playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(pl->guid);
                        playerController->GetPacket(ID_PLAYER_DRAWSTATE)->Send(pl->guid);

                        playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->setPlayer(pl);
                        playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(pl);
                        playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(pl);
                        playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(pl);
                        playerController->GetPacket(ID_PLAYER_DRAWSTATE)->setPlayer(pl);

                        playerController->GetPacket(ID_PLAYER_DYNAMICSTATS)->Send(other->guid);
                        playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(other->guid);
                        playerController->GetPacket(ID_PLAYER_SKILL)->Send(other->guid);
                        playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(other->guid);
                        playerController->GetPacket(ID_PLAYER_DRAWSTATE)->Send(other->guid);

                        LOG_APPEND(Log::LOG_INFO, "- Finished information exchange with %s", other->npc.mName.c_str());
                    }
                });

                playerController->GetPacket(ID_PLAYER_POS)->setPlayer(&player);
                playerController->GetPacket(ID_PLAYER_POS)->Send();
                packet.setPlayer(&player);
                packet.Send(true); //send to other clients

                Script::Call<Script::CallbackIdentity("OnPlayerCellChange")>(player.getId());

                LOG_APPEND(Log::LOG_INFO, "- Finished processing ID_PLAYER_CELL_CHANGE", player.cell.getDescription().c_str());
            }
            else
                LOG_APPEND(Log::LOG_INFO, "- Ignored because %s is dead", player.npc.mName.c_str());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
