#ifndef OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP
#define OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP

#include "../WorldProcessor.hpp"

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
