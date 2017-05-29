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
            playerController = Networking::get().getPlayerPacketController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            CellController::get()->update(&player);
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCELLSTATE_HPP
