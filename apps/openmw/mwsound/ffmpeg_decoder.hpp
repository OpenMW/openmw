#ifndef GAME_SOUND_FFMPEG_DECODER_H
#define GAME_SOUND_FFMPEG_DECODER_H

#include <string>

extern "C"
{
#include <avcodec.h>
#include <avformat.h>
}

#include "sound_decoder.hpp"


namespace MWSound
{
    class FFmpeg_Decoder : public Sound_Decoder
    {
        virtual void open(const std::string &fname);
        virtual void close();

        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void rewind();

        FFmpeg_Decoder();
        virtual ~FFmpeg_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::FFmpeg_Decoder)
#endif
};

#endif
