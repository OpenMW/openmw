//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERCELLSTATE_HPP
#define OPENMW_PROCESSORPLAYERCELLSTATE_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"
#include "apps/openmw-mp/Networking.hpp"
#include "apps/openmw-mp/Script/Script.hpp"
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>

namespace mwmp
{
    class ProcessorPlayerCellState : public PlayerProcessor
    {
        PlayerPacketController *playerController;
    public:
        ProcessorPlayerCellState()
        {
            BPP_INIT(ID_PLAYER_CELL_STATE)
            playerController = Networking::get().getPlayerController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID, player.npc.mName.c_str());

            CellController::get()->update(&player);

            Script::Call<Script::CallbackIdentity("OnPlayerCellState")>(player.getId());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCELLSTATE_HPP
