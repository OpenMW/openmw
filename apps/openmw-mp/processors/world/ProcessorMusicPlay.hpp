#ifndef OPENMW_PROCESSORMUSICPLAY_HPP
#define OPENMW_PROCESSORMUSICPLAY_HPP

#include "../WorldProcessor.hpp"

namespace mwmp
{
    class ProcessorMusicPlay : public WorldProcessor
    {
    public:
        ProcessorMusicPlay()
        {
            BPP_INIT(ID_MUSIC_PLAY)
        }
    };
}

#endif //OPENMW_PROCESSORMUSICPLAY_HPP
