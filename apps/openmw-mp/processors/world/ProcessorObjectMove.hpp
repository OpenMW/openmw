#ifndef OPENMW_PROCESSOROBJECTMOVE_HPP
#define OPENMW_PROCESSOROBJECTMOVE_HPP

#include "../WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectMove : public WorldProcessor
    {
    public:
        ProcessorObjectMove()
        {
            BPP_INIT(ID_OBJECT_MOVE)
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTMOVE_HPP
