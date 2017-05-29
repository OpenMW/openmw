#ifndef OPENMW_PROCESSORPLAYERREGIONCHANGE_HPP
#define OPENMW_PROCESSORPLAYERREGIONCHANGE_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerRegionChange : public PlayerProcessor
    {
    public:
        ProcessorPlayerRegionChange()
        {
            BPP_INIT(ID_PLAYER_REGION_CHANGE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerRegionChange")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERREGIONCHANGE_HPP
