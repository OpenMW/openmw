#ifndef GAME_SOUND_FFMPEG_DECODER_H
#define GAME_SOUND_FFMPEG_DECODER_H

#include <cstdint>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

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

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <components/files/istreamptr.hpp>

#include <string>

#include "sound_decoder.hpp"

#define FFMPEG_5_OR_GREATER (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100))
#define FFMPEG_CONST_WRITEPACKET (LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 12, 100))

namespace MWSound
{
    struct AVIOContextDeleter
    {
        void operator()(AVIOContext* ptr) const;
    };

    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVIOContextDeleter>;

    struct AVFormatContextDeleter
    {
        void operator()(AVFormatContext* ptr) const;
    };

    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;

    struct AVCodecContextDeleter
    {
        void operator()(AVCodecContext* ptr) const;
    };

    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;

    struct AVFrameDeleter
    {
        void operator()(AVFrame* ptr) const;
    };

    using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;

    class FFmpeg_Decoder final : public Sound_Decoder
    {
        AVIOContextPtr mIoCtx;
        AVFormatContextPtr mFormatCtx;
        AVCodecContextPtr mCodecCtx;
        AVStream** mStream;

        AVPacket mPacket;
        AVFramePtr mFrame;

        std::size_t mFrameSize;
        std::size_t mFramePos;

        double mNextPts;

        SwrContext* mSwr;
        enum AVSampleFormat mOutputSampleFormat;
#if FFMPEG_5_OR_GREATER
        AVChannelLayout mOutputChannelLayout;
#else
        int64_t mOutputChannelLayout;
#endif
        uint8_t* mDataBuf;
        uint8_t** mFrameData;
        int mDataBufLen;

        bool getNextPacket();

        Files::IStreamPtr mDataStream;

        static int readPacket(void* user_data, uint8_t* buf, int buf_size);
#if FFMPEG_CONST_WRITEPACKET
        static int writePacket(void* user_data, const uint8_t* buf, int buf_size);
#else
        static int writePacket(void* user_data, uint8_t* buf, int buf_size);
#endif
        static int64_t seek(void* user_data, int64_t offset, int whence);

        bool getAVAudioData();
        size_t readAVAudioData(void* data, size_t length);

        void open(VFS::Path::NormalizedView fname) override;
        void close() override;

        std::string getName() override;
        void getInfo(int* samplerate, ChannelConfig* chans, SampleType* type) override;

        size_t read(char* buffer, size_t bytes) override;
        void readAll(std::vector<char>& output) override;
        size_t getSampleOffset() override;

        FFmpeg_Decoder& operator=(const FFmpeg_Decoder& rhs);
        FFmpeg_Decoder(const FFmpeg_Decoder& rhs);

    public:
        explicit FFmpeg_Decoder(const VFS::Manager* vfs);

        virtual ~FFmpeg_Decoder();

        friend class SoundManager;
    };
}

#endif
