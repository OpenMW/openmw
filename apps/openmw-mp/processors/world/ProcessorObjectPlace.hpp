//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTPLACE_HPP
#define OPENMW_PROCESSOROBJECTPLACE_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"
#include <apps/openmw-mp/Networking.hpp>

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
            for (unsigned int i = 0; i < event.objectChanges.count; i++)
            {
                event.objectChanges.objects.at(i).mpNum = mwmp::Networking::getPtr()->getNextMpNum();
            }

            // Send this packet back to the original sender with the mpNum generation from above,
            // then send it to the other players
            packet.Send(false);
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnObjectPlace")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTPLACE_HPP
