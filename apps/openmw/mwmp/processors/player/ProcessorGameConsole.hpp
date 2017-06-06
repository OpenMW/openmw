//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORGAMECONSOLE_HPP
#define OPENMW_PROCESSORGAMECONSOLE_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGameConsole : public PlayerProcessor
    {
    public:
        ProcessorGameConsole()
        {
            BPP_INIT(ID_GAME_CONSOLE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {

        }
    };
}



#endif //OPENMW_PROCESSORGAMECONSOLE_HPP
