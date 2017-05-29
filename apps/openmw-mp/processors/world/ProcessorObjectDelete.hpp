#ifndef OPENMW_PROCESSOROBJECTDELETE_HPP
#define OPENMW_PROCESSOROBJECTDELETE_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectDelete : public WorldProcessor
    {
    public:
        ProcessorObjectDelete()
        {
            BPP_INIT(ID_OBJECT_DELETE)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());
            
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectDelete")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTDELETE_HPP
