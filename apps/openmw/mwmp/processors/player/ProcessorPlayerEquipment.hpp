//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
#define OPENMW_PROCESSORPLAYEREQUIPMENT_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

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
                if (isRequest())
                    static_cast<LocalPlayer*>(player)->updateEquipment(true);
                else
                    static_cast<LocalPlayer*>(player)->setEquipment();
            }
            else
                static_cast<DedicatedPlayer*>(player)->updateEquipment();
        }
    };
}

#endif //OPENMW_PROCESSORPLAYEREQUIPMENT_HPP
