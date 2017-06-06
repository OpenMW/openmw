#ifndef OPENMW_PROCESSORPLAYERCELLSTATE_HPP
#define OPENMW_PROCESSORPLAYERCELLSTATE_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerCellState : public PlayerProcessor
    {
    public:
        ProcessorPlayerCellState()
        {
            BPP_INIT(ID_PLAYER_CELL_STATE)
            avoidReading = true;
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal() && isRequest())
                static_cast<LocalPlayer *>(player)->sendCellStates();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERCELLSTATE_HPP
