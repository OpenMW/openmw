//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP
#define OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerStatsDynamic : public PlayerProcessor
    {
    public:
        ProcessorPlayerStatsDynamic()
        {
            BPP_INIT(ID_PLAYER_STATS_DYNAMIC)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if (isRequest())
                    static_cast<LocalPlayer *>(player)->updateStatsDynamic(true);
                else
                    static_cast<LocalPlayer *>(player)->setDynamicStats();
            }
            else if (player != 0)
            {
                MWWorld::Ptr ptrPlayer = static_cast<DedicatedPlayer*>(player)->getPtr();
                MWMechanics::CreatureStats *ptrCreatureStats = &ptrPlayer.getClass().getCreatureStats(ptrPlayer);
                MWMechanics::DynamicStat<float> value;

                for (int i = 0; i < 3; ++i)
                {
                    value.readState(player->creatureStats.mDynamic[i]);
                    ptrCreatureStats->setDynamic(i, value);
                }
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSTATSDYNAMIC_HPP
