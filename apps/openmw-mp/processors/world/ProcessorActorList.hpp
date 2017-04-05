#ifndef OPENMW_PROCESSORACTORLIST_HPP
#define OPENMW_PROCESSORACTORLIST_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorActorList : public WorldProcessor
    {
    public:
        ProcessorActorList()
        {
            BPP_INIT(ID_ACTOR_LIST)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);

            Script::Call<Script::CallbackIdentity("OnActorList")>(player.getId(), event.cell.getDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORACTORLIST_HPP
