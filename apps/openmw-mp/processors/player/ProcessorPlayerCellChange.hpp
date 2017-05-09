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
            playerController = Networking::get().getPlayerPacketController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            if (!player.creatureStats.mDead)
            {
                LOG_APPEND(Log::LOG_INFO, "- Moved to %s", player.cell.getDescription().c_str());

                player.forEachLoaded([this](Player *pl, Player *other) {

                    LOG_APPEND(Log::LOG_INFO, "- Started information exchange with %s", other->npc.mName.c_str());

                    playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(other);
                    playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(other);
                    playerController->GetPacket(ID_PLAYER_POSITION)->setPlayer(other);
                    playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(other);
                    playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(other);
                    playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(other);

                    playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(pl->guid);
                    playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(pl->guid);
                    playerController->GetPacket(ID_PLAYER_POSITION)->Send(pl->guid);
                    playerController->GetPacket(ID_PLAYER_SKILL)->Send(pl->guid);
                    playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(pl->guid);
                    playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->Send(pl->guid);

                    playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(pl);
                    playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(pl);
                    playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(pl);
                    playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(pl);
                    playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(pl);

                    playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(other->guid);
                    playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(other->guid);
                    playerController->GetPacket(ID_PLAYER_SKILL)->Send(other->guid);
                    playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(other->guid);
                    playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->Send(other->guid);

                    LOG_APPEND(Log::LOG_INFO, "- Finished information exchange with %s", other->npc.mName.c_str());
                });

                playerController->GetPacket(ID_PLAYER_POSITION)->setPlayer(&player);
                playerController->GetPacket(ID_PLAYER_POSITION)->Send();
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
