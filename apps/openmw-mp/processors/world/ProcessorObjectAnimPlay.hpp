//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSOROBJECTANIMPLAY_HPP
#define OPENMW_PROCESSOROBJECTANIMPLAY_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectAnimPlay : public WorldProcessor
    {
    public:
        ProcessorObjectAnimPlay()
        {
            BPP_INIT(ID_OBJECT_ANIM_PLAY)
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTANIMPLAY_HPP
