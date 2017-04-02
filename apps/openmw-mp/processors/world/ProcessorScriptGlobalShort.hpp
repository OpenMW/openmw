//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP
#define OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorScriptGlobalShort : public WorldProcessor
    {
    public:
        ProcessorScriptGlobalShort()
        {
            BPP_INIT(ID_SCRIPT_GLOBAL_SHORT)
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP
