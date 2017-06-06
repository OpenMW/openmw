#ifndef OPENMW_PROCESSORPLAYERBOOK_HPP
#define OPENMW_PROCESSORPLAYERBOOK_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerBook : public PlayerProcessor
    {
    public:
        ProcessorPlayerBook()
        {
            BPP_INIT(ID_PLAYER_BOOK)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            // Placeholder to be filled in later
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERBOOK_HPP
