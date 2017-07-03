#ifndef OPENMW_PROCESSORPLAYERBOUNTY_HPP
#define OPENMW_PROCESSORPLAYERBOUNTY_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerBounty : public PlayerProcessor
    {
    public:
        ProcessorPlayerBounty()
        {
            BPP_INIT(ID_PLAYER_BOUNTY)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            Script::Call<Script::CallbackIdentity("OnPlayerBounty")>(player.getId());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERBOUNTY_HPP
