#ifndef OPENMW_PROCESSORPLAYERBOUNTY_HPP
#define OPENMW_PROCESSORPLAYERBOUNTY_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerBounty : public PlayerProcessor
    {
    public:
        ProcessorPlayerBounty()
        {
            BPP_INIT(ID_PLAYER_BOUNTY)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                if(isRequest())
                    static_cast<LocalPlayer *>(player)->updateBounty(true);
                else
                    static_cast<LocalPlayer *>(player)->setBounty();
            }
            else if (player != 0)
            {
                MWWorld::Ptr ptrPlayer =  static_cast<DedicatedPlayer *>(player)->getPtr();
                MWMechanics::NpcStats *ptrNpcStats = &ptrPlayer.getClass().getNpcStats(ptrPlayer);

                ptrNpcStats->setBounty(player->npcStats.mBounty);
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERBOUNTY_HPP
