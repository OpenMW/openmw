//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTLOCK_HPP
#define OPENMW_PROCESSOROBJECTLOCK_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectLock : public WorldProcessor
    {
    public:
        ProcessorObjectLock()
        {
            BPP_INIT(ID_OBJECT_LOCK)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectLock")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTLOCK_HPP
