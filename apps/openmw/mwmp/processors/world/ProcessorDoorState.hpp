#ifndef OPENMW_PROCESSDOORSTATE_HPP
#define OPENMW_PROCESSDOORSTATE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorDoorState : public BaseObjectProcessor
    {
    public:
        ProcessorDoorState()
        {
            BPP_INIT(ID_DOOR_STATE)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.activateDoors(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSDOORSTATE_HPP
