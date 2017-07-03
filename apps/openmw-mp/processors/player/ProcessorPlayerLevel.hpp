#ifndef OPENMW_PROCESSORPLAYERLEVEL_HPP
#define OPENMW_PROCESSORPLAYERLEVEL_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerLevel : public PlayerProcessor
    {
    public:
        ProcessorPlayerLevel()
        {
            BPP_INIT(ID_PLAYER_LEVEL)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            if (!player.creatureStats.mDead)
            {
                Script::Call<Script::CallbackIdentity("OnPlayerLevel")>(player.getId());
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERLEVEL_HPP
