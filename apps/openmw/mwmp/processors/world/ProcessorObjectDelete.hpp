#ifndef OPENMW_PROCESSOROBJECTDELETE_HPP
#define OPENMW_PROCESSOROBJECTDELETE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectDelete : public BaseObjectProcessor
    {
    public:
        ProcessorObjectDelete()
        {
            BPP_INIT(ID_OBJECT_DELETE)
        }

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            BaseObjectProcessor::Do(packet, event);

            event.deleteObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTDELETE_HPP
