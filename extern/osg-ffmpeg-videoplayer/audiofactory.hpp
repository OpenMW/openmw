#ifndef VIDEO_MOVIEAUDIOFACTORY_H
#define VIDEO_MOVIEAUDIOFACTORY_H

#include "audiodecoder.hpp"


namespace Video
{

class MovieAudioFactory
{
public:
    virtual std::shared_ptr<MovieAudioDecoder> createDecoder(VideoState* videoState) = 0;
    virtual ~MovieAudioFactory() {}
};

}

#endif
