//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
#define OPENMW_PROCESSORPLAYEREQUIPMENT_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerEquipment : public PlayerProcessor
    {
    public:
        ProcessorPlayerEquipment()
        {
            BPP_INIT(ID_PLAYER_EQUIPMENT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.setPlayer(&player);
            packet.Read();
            //myPacket->Send(player, true);

            player.sendToLoaded(&packet);

            Script::Call<Script::CallbackIdentity("OnPlayerEquipmentChange")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
