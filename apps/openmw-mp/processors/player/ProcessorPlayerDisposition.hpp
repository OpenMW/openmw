#ifndef OPENMW_PROCESSORPLAYERDISPOSITION_HPP
#define OPENMW_PROCESSORPLAYERDISPOSITION_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDisposition : public PlayerProcessor
    {
    public:
        ProcessorPlayerDisposition()
        {
            BPP_INIT(ID_PLAYER_DISPOSITION)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerDisposition")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDISPOSITION_HPP
