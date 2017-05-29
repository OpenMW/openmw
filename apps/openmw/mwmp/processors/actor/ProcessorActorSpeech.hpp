#ifndef OPENMW_PROCESSORACTORSPEECH_HPP
#define OPENMW_PROCESSORACTORSPEECH_HPP

#include "apps/openmw/mwmp/ActorProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"

namespace mwmp
{
    class ProcessorActorSpeech : public ActorProcessor
    {
    public:
        ProcessorActorSpeech()
        {
            BPP_INIT(ID_ACTOR_SPEECH);
        }

        virtual void Do(ActorPacket &packet, ActorList &actorList)
        {
            Main::get().getCellController()->readSpeech(actorList);
        }
    };
}

#endif //OPENMW_PROCESSORACTORSPEECH_HPP
