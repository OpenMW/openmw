#ifndef GAME_SOUND_FFMPEG_DECODER_H
#define GAME_SOUND_FFMPEG_DECODER_H

#include <stdint.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>

// From version 54.56 binkaudio encoding format changed from S16 to FLTP. See:
// https://gitorious.org/ffmpeg/ffmpeg/commit/7bfd1766d1c18f07b0a2dd042418a874d49ea60d
// https://ffmpeg.zeranoe.com/forum/viewtopic.php?f=15&t=872
#include <libswresample/swresample.h>
}

#include <components/files/constrainedfilestream.hpp>

#include <string>
#include <istream>

#include "sound_decoder.hpp"


namespace MWSound
{
    class FFmpeg_Decoder final : public Sound_Decoder
    {
        AVFormatContext *mFormatCtx;
        AVCodecContext *mCodecCtx;
        AVStream **mStream;

        AVPacket mPacket;
        AVFrame *mFrame;

        int mFrameSize;
        int mFramePos;

        double mNextPts;

        SwrContext *mSwr;
        enum AVSampleFormat mOutputSampleFormat;
        int64_t mOutputChannelLayout;
        uint8_t *mDataBuf;
        uint8_t **mFrameData;
        int mDataBufLen;

        bool getNextPacket();

        Files::IStreamPtr mDataStream;

        static int readPacket(void *user_data, uint8_t *buf, int buf_size);
        static int writePacket(void *user_data, uint8_t *buf, int buf_size);
        static int64_t seek(void *user_data, int64_t offset, int whence);

        bool getAVAudioData();
        size_t readAVAudioData(void *data, size_t length);

        void open(const std::string &fname) override;
        void close() override;

        std::string getName() override;
        void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type) override;

        size_t read(char *buffer, size_t bytes) override;
        void readAll(std::vector<char> &output) override;
        size_t getSampleOffset() override;

        FFmpeg_Decoder& operator=(const FFmpeg_Decoder &rhs);
        FFmpeg_Decoder(const FFmpeg_Decoder &rhs);

        FFmpeg_Decoder(const VFS::Manager* vfs);
    public:
        virtual ~FFmpeg_Decoder();

        friend class SoundManager;
    };
#ifndef DEFAULT_DECODER
#define DEFAULT_DECODER (::MWSound::FFmpeg_Decoder)
#endif
}

#endif
