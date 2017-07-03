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

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerBook")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERBOOK_HPP
