#ifndef OPENMW_PROCESSOROBJECTSPAWN_HPP
#define OPENMW_PROCESSOROBJECTSPAWN_HPP

#include "../WorldProcessor.hpp"
#include <apps/openmw-mp/Networking.hpp>

namespace mwmp
{
    class ProcessorObjectSpawn : public WorldProcessor
    {
    public:
        ProcessorObjectSpawn()
        {
            BPP_INIT(ID_OBJECT_SPAWN)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            for (unsigned int i = 0; i < event.worldObjectCount; i++)
            {
                event.worldObjects.at(i).mpNum = mwmp::Networking::getPtr()->incrementMpNum();
            }

            // Send this packet back to the original sender with the mpNum generation from above,
            // then send it to the other players
            packet.Send(false);
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectSpawn")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSPAWN_HPP
