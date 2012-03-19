#ifdef OPENMW_USE_FFMPEG

#include "ffmpeg_decoder.hpp"


namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("FFmpeg exception: "+msg); }

void FFmpeg_Decoder::open(const std::string &fname)
{
    fail("Not currently working");
}

void FFmpeg_Decoder::close()
{
}

void FFmpeg_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    fail("Not currently working");
}

size_t FFmpeg_Decoder::read(char *buffer, size_t bytes)
{
    fail("Not currently working");
    return 0;
}

void FFmpeg_Decoder::rewind()
{
    fail("Not currently working");
}

FFmpeg_Decoder::FFmpeg_Decoder()
{
}

FFmpeg_Decoder::~FFmpeg_Decoder()
{
    close();
}

}

#endif
