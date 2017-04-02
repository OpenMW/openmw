//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP
#define OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorScriptMemberShort : public WorldProcessor
    {
    public:
        ProcessorScriptMemberShort()
        {
            BPP_INIT(ID_SCRIPT_MEMBER_SHORT)
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP
