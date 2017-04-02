//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTUNLOCK_HPP
#define OPENMW_PROCESSOROBJECTUNLOCK_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectUnlock : public WorldProcessor
    {
    public:
        ProcessorObjectUnlock()
        {
            BPP_INIT(ID_OBJECT_UNLOCK)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectUnlock")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTUNLOCK_HPP
