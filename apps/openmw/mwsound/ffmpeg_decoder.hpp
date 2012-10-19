#ifndef GAME_SOUND_FFMPEG_DECODER_H
#define GAME_SOUND_FFMPEG_DECODER_H

#include <string>

// FIXME: This can't be right? The headers refuse to build without UINT64_C,
// which only gets defined in stdint.h in either C99 mode or with this macro
// defined...
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
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
        AVFormatContext *mFormatCtx;

        struct MyStream;
        std::vector<MyStream*> mStreams;

        bool getNextPacket(int streamidx);

        Ogre::DataStreamPtr mDataStream;
        static int readPacket(void *user_data, uint8_t *buf, int buf_size);
        static int writePacket(void *user_data, uint8_t *buf, int buf_size);
        static int64_t seek(void *user_data, int64_t offset, int whence);

        virtual void open(const std::string &fname);
        virtual void close();

        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void readAll(std::vector<char> &output);
        virtual void rewind();

        FFmpeg_Decoder& operator=(const FFmpeg_Decoder &rhs);
        FFmpeg_Decoder(const FFmpeg_Decoder &rhs);

        FFmpeg_Decoder();
    public:
        virtual ~FFmpeg_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::FFmpeg_Decoder)
#endif
}

#endif
