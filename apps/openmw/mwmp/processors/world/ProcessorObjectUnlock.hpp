//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTUNLOCK_HPP
#define OPENMW_PROCESSOROBJECTUNLOCK_HPP


#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectUnlock : public BaseObjectProcessor
    {
    public:
        ProcessorObjectUnlock()
        {
            BPP_INIT(ID_OBJECT_UNLOCK)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.unlockObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTUNLOCK_HPP
