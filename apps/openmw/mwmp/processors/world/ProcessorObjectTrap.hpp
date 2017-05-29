#ifndef OPENMW_PROCESSOROBJECTTRAP_HPP
#define OPENMW_PROCESSOROBJECTTRAP_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectTrap : public BaseObjectProcessor
    {
    public:
        ProcessorObjectTrap()
        {
            BPP_INIT(ID_OBJECT_TRAP)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.triggerTrapObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTTRAP_HPP
