#ifndef OPENMW_PROCESSOROBJECTROTATE_HPP
#define OPENMW_PROCESSOROBJECTROTATE_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectRotate : public WorldProcessor
    {
    public:
        ProcessorObjectRotate()
        {
            BPP_INIT(ID_OBJECT_ROTATE)
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTROTATE_HPP
