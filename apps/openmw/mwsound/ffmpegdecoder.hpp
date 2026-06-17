#ifndef GAME_SOUND_FFMPEGDECODER_H
#define GAME_SOUND_FFMPEGDECODER_H

#include <cstdint>

#include <osg-ffmpeg-videoplayer/libavformatdefines.hpp>
#include <osg-ffmpeg-videoplayer/libavutildefines.hpp>

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

#include "sounddecoder.hpp"

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

    class FFmpegDecoder final : public SoundDecoder
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
#if OPENMW_FFMPEG_5_OR_GREATER
        AVChannelLayout mOutputChannelLayout;
#else
        int64_t mOutputChannelLayout;
#endif
        uint8_t* mDataBuf;
        uint8_t** mFrameData;
        int mDataBufLen;

        bool getNextPacket();

        Files::IStreamPtr mDataStream;

        static int readPacket(void* userData, uint8_t* buf, int bufSize);
#if OPENMW_FFMPEG_CONST_WRITEPACKET
        static int writePacket(void* userData, const uint8_t* buf, int bufSize);
#else
        static int writePacket(void* userData, uint8_t* buf, int bufSize);
#endif
        static int64_t seek(void* userData, int64_t offset, int whence);

        bool getAVAudioData();
        size_t readAVAudioData(void* data, size_t length);

        void open(VFS::Path::NormalizedView fname) override;
        void close() override;

        std::string getName() override;
        void getInfo(int* samplerate, ChannelConfig* chans, SampleType* type) override;

        size_t read(char* buffer, size_t bytes) override;
        void readAll(std::vector<char>& output) override;
        size_t getSampleOffset() override;

        FFmpegDecoder& operator=(const FFmpegDecoder& rhs);
        FFmpegDecoder(const FFmpegDecoder& rhs);

    public:
        explicit FFmpegDecoder(const VFS::Manager* vfs);

        virtual ~FFmpegDecoder();

        friend class SoundManager;
    };
}

#endif
