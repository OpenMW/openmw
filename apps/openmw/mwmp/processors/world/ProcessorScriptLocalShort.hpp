//
// Created by koncord on 18.04.17.
//

#ifndef OPENMW_PROCESSORSCRIPTLOCALSHORT_HPP
#define OPENMW_PROCESSORSCRIPTLOCALSHORT_HPP


#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorScriptLocalShort : public BaseObjectProcessor
    {
    public:
        ProcessorScriptLocalShort()
        {
            BPP_INIT(ID_SCRIPT_LOCAL_SHORT)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.setLocalShorts(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTLOCALSHORT_HPP
