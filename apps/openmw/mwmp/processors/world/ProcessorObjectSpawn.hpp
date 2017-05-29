#ifndef OPENMW_PROCESSOROBJECTSPAWN_HPP
#define OPENMW_PROCESSOROBJECTSPAWN_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectSpawn : public BaseObjectProcessor
    {
    public:
        ProcessorObjectSpawn()
        {
            BPP_INIT(ID_OBJECT_SPAWN)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.spawnObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSPAWN_HPP
