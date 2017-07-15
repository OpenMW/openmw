#ifndef OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP
#define OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerShapeshift : public PlayerProcessor
    {
    public:
        ProcessorPlayerShapeshift()
        {
            BPP_INIT(ID_PLAYER_SHAPESHIFT)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnPlayerShapeshift")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP
