#ifdef OPENMW_USE_FFMPEG

#include "ffmpeg_decoder.hpp"


namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("FFmpeg exception: "+msg); }

bool FFmpeg_Decoder::Open(const std::string &fname)
{
    fail("Not currently working");
    return false;
}

void FFmpeg_Decoder::Close()
{
}

void FFmpeg_Decoder::GetInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    fail("Not currently working");
}

size_t FFmpeg_Decoder::Read(char *buffer, size_t bytes)
{
    fail("Not currently working");
    return 0;
}


FFmpeg_Decoder::FFmpeg_Decoder()
{
}

FFmpeg_Decoder::~FFmpeg_Decoder()
{
}

}

#endif
