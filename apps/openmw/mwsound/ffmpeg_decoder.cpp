#ifdef OPENMW_USE_FFMPEG

#include "ffmpeg_decoder.hpp"


namespace MWSound
{

bool FFmpeg_Decoder::Open(const std::string &fname)
{
    return false;
}

void FFmpeg_Decoder::Close()
{
}


FFmpeg_Decoder::FFmpeg_Decoder()
{
}

FFmpeg_Decoder::~FFmpeg_Decoder()
{
}

}

#endif
