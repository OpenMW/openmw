//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTSCALE_HPP
#define OPENMW_PROCESSOROBJECTSCALE_HPP


#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectScale : public BaseObjectProcessor
    {
    public:
        ProcessorObjectScale()
        {
            BPP_INIT(ID_OBJECT_SCALE)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.scaleObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSCALE_HPP
