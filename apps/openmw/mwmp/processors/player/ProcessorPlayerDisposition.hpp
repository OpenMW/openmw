#ifndef OPENMW_PROCESSORPLAYERDISPOSITION_HPP
#define OPENMW_PROCESSORPLAYERDISPOSITION_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerDisposition : public PlayerProcessor
    {
    public:
        ProcessorPlayerDisposition()
        {
            BPP_INIT(ID_PLAYER_DISPOSITION)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERDISPOSITION_HPP
