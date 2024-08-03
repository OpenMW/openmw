#include "ffmpeg_decoder.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include <components/debug/debuglog.hpp>
#include <components/vfs/manager.hpp>

#if FFMPEG_5_OR_GREATER
#include <libavutil/channel_layout.h>
#endif

namespace MWSound
{
    void AVIOContextDeleter::operator()(AVIOContext* ptr) const
    {
        if (ptr->buffer != nullptr)
            av_freep(&ptr->buffer);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 80, 100)
        avio_context_free(&ptr);
#else
        av_free(ptr);
#endif
    }

    void AVFormatContextDeleter::operator()(AVFormatContext* ptr) const
    {
        avformat_close_input(&ptr);
    }

    void AVCodecContextDeleter::operator()(AVCodecContext* ptr) const
    {
        avcodec_free_context(&ptr);
    }

    void AVFrameDeleter::operator()(AVFrame* ptr) const
    {
        av_frame_free(&ptr);
    }

    int FFmpeg_Decoder::readPacket(void* user_data, uint8_t* buf, int buf_size)
    {
        try
        {
            std::istream& stream = *static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;
            stream.clear();
            stream.read((char*)buf, buf_size);
            std::streamsize count = stream.gcount();
            if (count == 0)
                return AVERROR_EOF;
            if (count > std::numeric_limits<int>::max())
                return AVERROR_BUG;
            return static_cast<int>(count);
        }
        catch (std::exception&)
        {
            return AVERROR_UNKNOWN;
        }
    }

#if FFMPEG_CONST_WRITEPACKET
    int FFmpeg_Decoder::writePacket(void*, const uint8_t*, int)
#else
    int FFmpeg_Decoder::writePacket(void*, uint8_t*, int)
#endif
    {
        Log(Debug::Error) << "can't write to read-only stream";
        return -1;
    }

    int64_t FFmpeg_Decoder::seek(void* user_data, int64_t offset, int whence)
    {
        std::istream& stream = *static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;

        whence &= ~AVSEEK_FORCE;

        stream.clear();

        if (whence == AVSEEK_SIZE)
        {
            size_t prev = stream.tellg();
            stream.seekg(0, std::ios_base::end);
            size_t size = stream.tellg();
            stream.seekg(prev, std::ios_base::beg);
            return size;
        }

        if (whence == SEEK_SET)
            stream.seekg(offset, std::ios_base::beg);
        else if (whence == SEEK_CUR)
            stream.seekg(offset, std::ios_base::cur);
        else if (whence == SEEK_END)
            stream.seekg(offset, std::ios_base::end);
        else
            return -1;

        return stream.tellg();
    }

    /* Used by getAV*Data to search for more compressed data, and buffer it in the
     * correct stream. It won't buffer data for streams that the app doesn't have a
     * handle for. */
    bool FFmpeg_Decoder::getNextPacket()
    {
        if (!mStream)
            return false;

        std::ptrdiff_t stream_idx = mStream - mFormatCtx->streams;
        while (av_read_frame(mFormatCtx.get(), &mPacket) >= 0)
        {
            /* Check if the packet belongs to this stream */
            if (stream_idx == mPacket.stream_index)
            {
                if (mPacket.pts != (int64_t)AV_NOPTS_VALUE)
                    mNextPts = av_q2d((*mStream)->time_base) * mPacket.pts;
                return true;
            }

            /* Free the packet and look for another */
            av_packet_unref(&mPacket);
        }

        return false;
    }

    bool FFmpeg_Decoder::getAVAudioData()
    {
        bool got_frame = false;

        if (mCodecCtx->codec_type != AVMEDIA_TYPE_AUDIO)
            return false;

        do
        {
            /* Decode some data, and check for errors */
            int ret = avcodec_receive_frame(mCodecCtx.get(), mFrame.get());
            if (ret == AVERROR(EAGAIN))
            {
                if (mPacket.size == 0 && !getNextPacket())
                    return false;
                ret = avcodec_send_packet(mCodecCtx.get(), &mPacket);
                av_packet_unref(&mPacket);
                if (ret == 0)
                    continue;
            }
            if (ret != 0)
                return false;

            av_packet_unref(&mPacket);

            if (mFrame->nb_samples == 0)
                continue;
            got_frame = true;

            if (mSwr)
            {
                if (!mDataBuf || mDataBufLen < mFrame->nb_samples)
                {
                    av_freep(&mDataBuf);
#if FFMPEG_5_OR_GREATER
                    if (av_samples_alloc(&mDataBuf, nullptr, mOutputChannelLayout.nb_channels,
#else
                    if (av_samples_alloc(&mDataBuf, nullptr, av_get_channel_layout_nb_channels(mOutputChannelLayout),
#endif
                            mFrame->nb_samples, mOutputSampleFormat, 0)
                        < 0)
                        return false;
                    else
                        mDataBufLen = mFrame->nb_samples;
                }

                if (swr_convert(mSwr, (uint8_t**)&mDataBuf, mFrame->nb_samples, (const uint8_t**)mFrame->extended_data,
                        mFrame->nb_samples)
                    < 0)
                {
                    return false;
                }
                mFrameData = &mDataBuf;
            }
            else
                mFrameData = &mFrame->data[0];

        } while (!got_frame);
        mNextPts += (double)mFrame->nb_samples / mCodecCtx->sample_rate;

        return true;
    }

    size_t FFmpeg_Decoder::readAVAudioData(void* data, size_t length)
    {
        size_t dec = 0;

        while (dec < length)
        {
            /* If there's no decoded data, find some */
            if (mFramePos >= mFrameSize)
            {
                if (!getAVAudioData())
                    break;
                mFramePos = 0;
#if FFMPEG_5_OR_GREATER
                mFrameSize = mFrame->nb_samples * mOutputChannelLayout.nb_channels
#else
                mFrameSize = mFrame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout)
#endif
                    * av_get_bytes_per_sample(mOutputSampleFormat);
            }

            /* Get the amount of bytes remaining to be written, and clamp to
             * the amount of decoded data we have */
            size_t rem = std::min<size_t>(length - dec, mFrameSize - mFramePos);

            /* Copy the data to the app's buffer and increment */
            memcpy(data, mFrameData[0] + mFramePos, rem);
            data = (char*)data + rem;
            dec += rem;
            mFramePos += rem;
        }

        /* Return the number of bytes we were able to get */
        return dec;
    }

    void FFmpeg_Decoder::open(VFS::Path::NormalizedView fname)
    {
        close();
        mDataStream = mResourceMgr->get(fname);

        AVIOContextPtr ioCtx(avio_alloc_context(nullptr, 0, 0, this, readPacket, writePacket, seek));
        if (ioCtx == nullptr)
            throw std::runtime_error("Failed to allocate AVIO context");

        AVFormatContext* formatCtx = avformat_alloc_context();
        if (formatCtx == nullptr)
            throw std::runtime_error("Failed to allocate context");

        formatCtx->pb = ioCtx.get();

        // avformat_open_input frees user supplied AVFormatContext on failure
        if (avformat_open_input(&formatCtx, fname.value().data(), nullptr, nullptr) != 0)
            throw std::runtime_error("Failed to open input");

        AVFormatContextPtr formatCtxPtr(std::exchange(formatCtx, nullptr));

        if (avformat_find_stream_info(formatCtxPtr.get(), nullptr) < 0)
            throw std::runtime_error("Failed to find stream info");

        AVStream** stream = nullptr;
        for (size_t j = 0; j < formatCtxPtr->nb_streams; j++)
        {
            if (formatCtxPtr->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                stream = &formatCtxPtr->streams[j];
                break;
            }
        }

        if (stream == nullptr)
            throw std::runtime_error("No audio streams");

        const AVCodec* codec = avcodec_find_decoder((*stream)->codecpar->codec_id);
        if (codec == nullptr)
            throw std::runtime_error("No codec found for id " + std::to_string((*stream)->codecpar->codec_id));

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx == nullptr)
            throw std::runtime_error("Failed to allocate codec context");

        avcodec_parameters_to_context(codecCtx, (*stream)->codecpar);

// This is not needed anymore above FFMpeg version 4.0
#if LIBAVCODEC_VERSION_INT < 3805796
        av_codec_set_pkt_timebase(avctx, (*stream)->time_base);
#endif

        AVCodecContextPtr codecCtxPtr(std::exchange(codecCtx, nullptr));

        if (avcodec_open2(codecCtxPtr.get(), codec, nullptr) < 0)
            throw std::runtime_error(std::string("Failed to open audio codec ") + codec->long_name);

        AVFramePtr frame(av_frame_alloc());
        if (frame == nullptr)
            throw std::runtime_error("Failed to allocate frame");

        if (codecCtxPtr->sample_fmt == AV_SAMPLE_FMT_U8P)
            mOutputSampleFormat = AV_SAMPLE_FMT_U8;
        // FIXME: Check for AL_EXT_FLOAT32 support
        // else if (codecCtxPtr->sample_fmt == AV_SAMPLE_FMT_FLT || codecCtxPtr->sample_fmt == AV_SAMPLE_FMT_FLTP)
        //     mOutputSampleFormat = AV_SAMPLE_FMT_S16;
        else
            mOutputSampleFormat = AV_SAMPLE_FMT_S16;

#if FFMPEG_5_OR_GREATER
        mOutputChannelLayout = (*stream)->codecpar->ch_layout; // sefault
        if (mOutputChannelLayout.u.mask == 0)
            av_channel_layout_default(&mOutputChannelLayout, codecCtxPtr->ch_layout.nb_channels);

        codecCtxPtr->ch_layout = mOutputChannelLayout;
#else
        mOutputChannelLayout = (*stream)->codecpar->channel_layout;
        if (mOutputChannelLayout == 0)
            mOutputChannelLayout = av_get_default_channel_layout(codecCtxPtr->channels);

        codecCtxPtr->channel_layout = mOutputChannelLayout;
#endif

        mIoCtx = std::move(ioCtx);
        mFrame = std::move(frame);
        mFormatCtx = std::move(formatCtxPtr);
        mCodecCtx = std::move(codecCtxPtr);
        mStream = stream;
    }

    void FFmpeg_Decoder::close()
    {
        mStream = nullptr;
        mCodecCtx.reset();

        av_packet_unref(&mPacket);
        av_freep(&mDataBuf);
        mFrame.reset();
        swr_free(&mSwr);

        mFormatCtx.reset();
        mIoCtx.reset();
        mDataStream.reset();
    }

    std::string FFmpeg_Decoder::getName()
    {
// In the FFMpeg 4.0 a "filename" field was replaced by "url"
#if LIBAVCODEC_VERSION_INT < 3805796
        return mFormatCtx->filename;
#else
        return mFormatCtx->url;
#endif
    }

    void FFmpeg_Decoder::getInfo(int* samplerate, ChannelConfig* chans, SampleType* type)
    {
        if (!mStream)
            throw std::runtime_error("No audio stream info");

        if (mOutputSampleFormat == AV_SAMPLE_FMT_U8)
            *type = SampleType_UInt8;
        else if (mOutputSampleFormat == AV_SAMPLE_FMT_S16)
            *type = SampleType_Int16;
        else if (mOutputSampleFormat == AV_SAMPLE_FMT_FLT)
            *type = SampleType_Float32;
        else
        {
            mOutputSampleFormat = AV_SAMPLE_FMT_S16;
            *type = SampleType_Int16;
        }

#if FFMPEG_5_OR_GREATER
        switch (mOutputChannelLayout.u.mask)
#else
        switch (mOutputChannelLayout)
#endif
        {
            case AV_CH_LAYOUT_MONO:
                *chans = ChannelConfig_Mono;
                break;
            case AV_CH_LAYOUT_STEREO:
                *chans = ChannelConfig_Stereo;
                break;
            case AV_CH_LAYOUT_QUAD:
                *chans = ChannelConfig_Quad;
                break;
            case AV_CH_LAYOUT_5POINT1:
                *chans = ChannelConfig_5point1;
                break;
            case AV_CH_LAYOUT_7POINT1:
                *chans = ChannelConfig_7point1;
                break;
            default:
                char str[1024];
#if FFMPEG_5_OR_GREATER
                av_channel_layout_describe(&mCodecCtx->ch_layout, str, sizeof(str));
                Log(Debug::Error) << "Unsupported channel layout: " << str;

                if (mCodecCtx->ch_layout.nb_channels == 1)
                {
                    mOutputChannelLayout = AV_CHANNEL_LAYOUT_MONO;
                    *chans = ChannelConfig_Mono;
                }
                else
                {
                    mOutputChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
                    *chans = ChannelConfig_Stereo;
                }
#else
                av_get_channel_layout_string(str, sizeof(str), mCodecCtx->channels, mCodecCtx->channel_layout);
                Log(Debug::Error) << "Unsupported channel layout: " << str;

                if (mCodecCtx->channels == 1)
                {
                    mOutputChannelLayout = AV_CH_LAYOUT_MONO;
                    *chans = ChannelConfig_Mono;
                }
                else
                {
                    mOutputChannelLayout = AV_CH_LAYOUT_STEREO;
                    *chans = ChannelConfig_Stereo;
                }
#endif
                break;
        }

        *samplerate = mCodecCtx->sample_rate;
#if FFMPEG_5_OR_GREATER
        AVChannelLayout ch_layout = mCodecCtx->ch_layout;
        if (ch_layout.u.mask == 0)
            av_channel_layout_default(&ch_layout, mCodecCtx->ch_layout.nb_channels);

        if (mOutputSampleFormat != mCodecCtx->sample_fmt || mOutputChannelLayout.u.mask != ch_layout.u.mask)
#else
        int64_t ch_layout = mCodecCtx->channel_layout;
        if (ch_layout == 0)
            ch_layout = av_get_default_channel_layout(mCodecCtx->channels);

        if (mOutputSampleFormat != mCodecCtx->sample_fmt || mOutputChannelLayout != ch_layout)
#endif

        {
#if FFMPEG_5_OR_GREATER
            swr_alloc_set_opts2(&mSwr, // SwrContext
                &mOutputChannelLayout, // output ch layout
                mOutputSampleFormat, // output sample format
                mCodecCtx->sample_rate, // output sample rate
                &ch_layout, // input ch layout
                mCodecCtx->sample_fmt, // input sample format
                mCodecCtx->sample_rate, // input sample rate
                0, // logging level offset
                nullptr); // log context
#else
            mSwr = swr_alloc_set_opts(mSwr, // SwrContext
                mOutputChannelLayout, // output ch layout
                mOutputSampleFormat, // output sample format
                mCodecCtx->sample_rate, // output sample rate
                ch_layout, // input ch layout
                mCodecCtx->sample_fmt, // input sample format
                mCodecCtx->sample_rate, // input sample rate
                0, // logging level offset
                nullptr); // log context
#endif
            if (!mSwr)
                throw std::runtime_error("Couldn't allocate SwrContext");
            int init = swr_init(mSwr);
            if (init < 0)
                throw std::runtime_error("Couldn't initialize SwrContext: " + std::to_string(init));
        }
    }

    size_t FFmpeg_Decoder::read(char* buffer, size_t bytes)
    {
        if (!mStream)
        {
            Log(Debug::Error) << "No audio stream";
            return 0;
        }
        return readAVAudioData(buffer, bytes);
    }

    void FFmpeg_Decoder::readAll(std::vector<char>& output)
    {
        if (!mStream)
        {
            Log(Debug::Error) << "No audio stream";
            return;
        }

        while (getAVAudioData())
        {
#if FFMPEG_5_OR_GREATER
            size_t got = mFrame->nb_samples * mOutputChannelLayout.nb_channels
#else
            size_t got = mFrame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout)
#endif
                * av_get_bytes_per_sample(mOutputSampleFormat);
            const char* inbuf = reinterpret_cast<char*>(mFrameData[0]);
            output.insert(output.end(), inbuf, inbuf + got);
        }
    }

    size_t FFmpeg_Decoder::getSampleOffset()
    {
#if FFMPEG_5_OR_GREATER
        std::size_t delay = (mFrameSize - mFramePos) / mOutputChannelLayout.nb_channels
#else
        std::size_t delay = (mFrameSize - mFramePos) / av_get_channel_layout_nb_channels(mOutputChannelLayout)
#endif
            / av_get_bytes_per_sample(mOutputSampleFormat);
        return static_cast<std::size_t>(mNextPts * mCodecCtx->sample_rate) - delay;
    }

    FFmpeg_Decoder::FFmpeg_Decoder(const VFS::Manager* vfs)
        : Sound_Decoder(vfs)
        , mStream(nullptr)
        , mFrameSize(0)
        , mFramePos(0)
        , mNextPts(0.0)
        , mSwr(nullptr)
        , mOutputSampleFormat(AV_SAMPLE_FMT_NONE)
#if FFMPEG_5_OR_GREATER
        , mOutputChannelLayout({})
#else
        , mOutputChannelLayout(0)
#endif
        , mDataBuf(nullptr)
        , mFrameData(nullptr)
        , mDataBufLen(0)
    {
        memset(&mPacket, 0, sizeof(mPacket));

        /* We need to make sure ffmpeg is initialized. Optionally silence warning
         * output from the lib */
        static bool done_init = false;
        if (!done_init)
        {
// This is not needed anymore above FFMpeg version 4.0
#if LIBAVCODEC_VERSION_INT < 3805796
            av_register_all();
#endif
            av_log_set_level(AV_LOG_ERROR);
            done_init = true;
        }
    }

    FFmpeg_Decoder::~FFmpeg_Decoder()
    {
        close();
    }
}
