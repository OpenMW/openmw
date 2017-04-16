//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERUPDATEINVENTORY_HPP
#define OPENMW_PROCESSORPLAYERUPDATEINVENTORY_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerInventory : public PlayerProcessor
    {
    public:
        ProcessorPlayerInventory()
        {
            BPP_INIT(ID_PLAYER_INVENTORY)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (!isLocal()) return;

            if (isRequest())
                static_cast<LocalPlayer*>(player)->updateInventory(true);
            else
            {
                LocalPlayer &localPlayer = static_cast<LocalPlayer&>(*player);
                int inventoryAction = localPlayer.inventoryChanges.action;

                if (inventoryAction == InventoryChanges::ADD)
                    localPlayer.addItems();
                else if (inventoryAction == InventoryChanges::REMOVE)
                    localPlayer.removeItems();
                else // InventoryChanges::SET
                    localPlayer.setInventory();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERUPDATEINVENTORY_HPP
