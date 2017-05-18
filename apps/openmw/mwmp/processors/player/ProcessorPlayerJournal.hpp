//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERJOURNAL_HPP
#define OPENMW_PROCESSORPLAYERJOURNAL_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerJournal : public PlayerProcessor
    {
    public:
        ProcessorPlayerJournal()
        {
            BPP_INIT(ID_PLAYER_JOURNAL)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isRequest())
            {
                // Entire journal cannot currently be requested from players
            }
            else
                static_cast<LocalPlayer*>(player)->addJournalItems();
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERJOURNAL_HPP
