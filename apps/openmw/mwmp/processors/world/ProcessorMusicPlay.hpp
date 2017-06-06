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

        virtual void Do(WorldPacket &packet, WorldEvent &event)
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Received %s", strPacketID.c_str());
            event.playMusic();
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTGLOBALSHORT_HPP
