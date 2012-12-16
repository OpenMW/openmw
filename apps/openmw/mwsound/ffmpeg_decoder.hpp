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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "sound_decoder.hpp"


namespace MWSound
{
    class FFmpeg_Decoder : public Sound_Decoder
    {
        AVFormatContext *mFormatCtx;
        AVStream **mStream;

        AVPacket mPacket;
        AVFrame *mFrame;

        int mFrameSize;
        int mFramePos;

        double mNextPts;

        bool getNextPacket();

        Ogre::DataStreamPtr mDataStream;
        static int readPacket(void *user_data, uint8_t *buf, int buf_size);
        static int writePacket(void *user_data, uint8_t *buf, int buf_size);
        static int64_t seek(void *user_data, int64_t offset, int whence);

        bool getAVAudioData();
        size_t readAVAudioData(void *data, size_t length);

        virtual void open(const std::string &fname);
        virtual void close();

        virtual std::string getName();
        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void readAll(std::vector<char> &output);
        virtual void rewind();
        virtual size_t getSampleOffset();

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
