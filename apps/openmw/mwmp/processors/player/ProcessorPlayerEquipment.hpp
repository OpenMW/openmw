//
// Created by koncord on 16.04.17.
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

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received ID_PLAYER_EQUIPMENT about LocalPlayer from server");

                if (isRequest())
                    static_cast<LocalPlayer*>(player)->updateEquipment(true);
                else
                    static_cast<LocalPlayer*>(player)->setEquipment();
            }
            else if (player != 0)
                static_cast<DedicatedPlayer*>(player)->setEquipment();
        }
    };
}

#endif //OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
