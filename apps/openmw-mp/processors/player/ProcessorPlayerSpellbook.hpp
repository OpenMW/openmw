//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERSPELLBOOK_HPP
#define OPENMW_PROCESSORPLAYERSPELLBOOK_HPP

#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerSpellbook : public PlayerProcessor
    {
    public:
        ProcessorPlayerSpellbook()
        {
            BPP_INIT(ID_PLAYER_SPELLBOOK)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());
            
            Script::Call<Script::CallbackIdentity("OnPlayerSpellbookChange")>(player.getId());
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERSPELLBOOK_HPP
