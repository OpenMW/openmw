//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTPLACE_HPP
#define OPENMW_PROCESSOROBJECTPLACE_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectPlace : public WorldProcessor
    {
    public:
        ProcessorObjectPlace()
        {
            BPP_INIT(ID_OBJECT_PLACE)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectPlace")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTPLACE_HPP
