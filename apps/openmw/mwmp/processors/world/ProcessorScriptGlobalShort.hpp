#ifndef OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP
#define OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP

#include "../WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorScriptGlobalShort : public WorldProcessor
    {
    public:
        ProcessorScriptGlobalShort()
        {
            BPP_INIT(ID_SCRIPT_GLOBAL_SHORT)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s", strPacketID.c_str());
            event.setGlobalShorts();
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP
