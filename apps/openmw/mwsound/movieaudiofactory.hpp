#ifndef OPENMW_MWSOUND_MOVIEAUDIOFACTORY_H
#define OPENMW_MWSOUND_MOVIEAUDIOFACTORY_H

#include <extern/osg-ffmpeg-videoplayer/audiofactory.hpp>

namespace MWSound
{

    class MovieAudioFactory : public Video::MovieAudioFactory
    {
        std::shared_ptr<Video::MovieAudioDecoder> createDecoder(Video::VideoState* videoState) override;
    };

}

#endif
