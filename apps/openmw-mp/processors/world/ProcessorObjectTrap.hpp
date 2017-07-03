#ifndef OPENMW_PROCESSOROBJECTTRAP_HPP
#define OPENMW_PROCESSOROBJECTTRAP_HPP

#include "../WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectTrap : public WorldProcessor
    {
    public:
        ProcessorObjectTrap()
        {
            BPP_INIT(ID_OBJECT_TRAP)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());
            
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectTrap")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTTRAP_HPP
