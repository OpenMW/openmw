//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTLOCK_HPP
#define OPENMW_PROCESSOROBJECTLOCK_HPP


#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectLock : public BaseObjectProcessor
    {
    public:
        ProcessorObjectLock()
        {
            BPP_INIT(ID_OBJECT_LOCK)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.lockObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTLOCK_HPP
