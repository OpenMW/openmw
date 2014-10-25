#ifndef VIDEO_MOVIEAUDIOFACTORY_H
#define VIDEO_MOVIEAUDIOFACTORY_H

#include "audiodecoder.hpp"

#include <boost/shared_ptr.hpp>

namespace Video
{

class MovieAudioFactory
{
public:
    virtual boost::shared_ptr<MovieAudioDecoder> createDecoder(VideoState* videoState) = 0;
    virtual ~MovieAudioFactory() {}
};

}

#endif
