#ifndef OPENMW_PROCESSOROBJECTMOVE_HPP
#define OPENMW_PROCESSOROBJECTMOVE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectMove : public BaseObjectProcessor
    {
    public:
        ProcessorObjectMove()
        {
            BPP_INIT(ID_OBJECT_MOVE)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.moveObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTMOVE_HPP
