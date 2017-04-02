//
// Created by koncord on 03.04.17.
//

#ifndef OPENMW_PROCESSORMUSICPLAY_HPP
#define OPENMW_PROCESSORMUSICPLAY_HPP

#include "apps/openmw-mp/WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorMusicPlay : public WorldProcessor
    {
    public:
        ProcessorMusicPlay()
        {
            BPP_INIT(ID_MUSIC_PLAY)
        }

        void Do(WorldPacket &packet, Player &player, BaseEvent &event) override
        {
            packet.Send(true);
        }
    };
}

#endif //OPENMW_PROCESSORMUSICPLAY_HPP
