//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTROTATE_HPP
#define OPENMW_PROCESSOROBJECTROTATE_HPP


#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectRotate : public BaseObjectProcessor
    {
    public:
        ProcessorObjectRotate()
        {
            BPP_INIT(ID_OBJECT_ROTATE)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.rotateObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTROTATE_HPP
