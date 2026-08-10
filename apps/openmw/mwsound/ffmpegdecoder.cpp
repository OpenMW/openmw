#include "ffmpegdecoder.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include <components/debug/debuglog.hpp>
#include <components/vfs/manager.hpp>

#include <osg-ffmpeg-videoplayer/libavformatdefines.hpp>
#include <osg-ffmpeg-videoplayer/libavutildefines.hpp>

#if OPENMW_FFMPEG_5_OR_GREATER
#include <libavutil/channel_layout.h>
#endif

#include "headcache.hpp"

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

    int FFmpegDecoder::readPacket(void* userData, uint8_t* buf, int bufSize)
    {
        try
        {
            std::istream& stream = *static_cast<FFmpegDecoder*>(userData)->mDataStream;
            stream.clear();
            stream.read((char*)buf, bufSize);
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

#if OPENMW_FFMPEG_CONST_WRITEPACKET
    int FFmpegDecoder::writePacket(void*, const uint8_t*, int)
#else
    int FFmpegDecoder::writePacket(void*, uint8_t*, int)
#endif
    {
        Log(Debug::Error) << "can't write to read-only stream";
        return -1;
    }

    int64_t FFmpegDecoder::seek(void* userData, int64_t offset, int whence)
    {
        std::istream& stream = *static_cast<FFmpegDecoder*>(userData)->mDataStream;

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
    bool FFmpegDecoder::getNextPacket()
    {
        if (!mStream)
            return false;

        std::ptrdiff_t streamIdx = mStream - mFormatCtx->streams;
        while (av_read_frame(mFormatCtx.get(), &mPacket) >= 0)
        {
            /* Check if the packet belongs to this stream */
            if (streamIdx == mPacket.stream_index)
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

    bool FFmpegDecoder::getAVAudioData()
    {
        bool gotFrame = false;

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
            gotFrame = true;

            if (mSwr)
            {
                if (!mDataBuf || mDataBufLen < mFrame->nb_samples)
                {
                    av_freep(&mDataBuf);
#if OPENMW_FFMPEG_5_OR_GREATER
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

        } while (!gotFrame);
        mNextPts += (double)mFrame->nb_samples / mCodecCtx->sample_rate;

        return true;
    }

    size_t FFmpegDecoder::readAVAudioData(void* data, size_t length)
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
#if OPENMW_FFMPEG_5_OR_GREATER
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

    namespace
    {
        AVStream** findAudioStream(AVFormatContext& ctx)
        {
            for (unsigned j = 0; j < ctx.nb_streams; j++)
                if (ctx.streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                    return &ctx.streams[j];
            return nullptr;
        }

        bool hasCompleteAudioParameters(const AVStream& stream)
        {
            const AVCodecParameters& par = *stream.codecpar;
#if OPENMW_FFMPEG_5_OR_GREATER
            const int channels = par.ch_layout.nb_channels;
#else
            const int channels = par.channels;
#endif
            return par.codec_id != AV_CODEC_ID_NONE && par.sample_rate > 0 && channels > 0;
        }

        const AVInputFormat* guessFormat(VFS::Path::NormalizedView fname)
        {
            const auto ext = fname.extension();
            if (ext == "wav")
                return av_find_input_format("wav");
            if (ext == "mp3")
                return av_find_input_format("mp3");
            if (ext == "ogg" || ext == "opus")
                return av_find_input_format("ogg");
            if (ext == "flac")
                return av_find_input_format("flac");
            return nullptr;
        }
    }

    bool FFmpegDecoder::openContext(const char* name, const AVInputFormat* fmt, bool limitProbe, AVIOContextPtr& ioCtx,
        AVFormatContextPtr& formatCtx, AVStream**& stream)
    {
        // A real IO buffer is required: with a named input format the demuxer
        // reads through it directly, and a zero-size buffer fails the open
        // (the probed path tolerates it, which hid this).
        constexpr int ioBufferSize = 8192;
        unsigned char* ioBuffer = static_cast<unsigned char*>(av_malloc(ioBufferSize));
        if (ioBuffer == nullptr)
            throw std::runtime_error("Failed to allocate AVIO buffer");
        ioCtx.reset(avio_alloc_context(ioBuffer, ioBufferSize, 0, this, readPacket, writePacket, seek));
        if (ioCtx == nullptr)
        {
            av_freep(&ioBuffer);
            throw std::runtime_error("Failed to allocate AVIO context");
        }

        AVFormatContext* ctx = avformat_alloc_context();
        if (ctx == nullptr)
            throw std::runtime_error("Failed to allocate context");

        ctx->pb = ioCtx.get();
        if (limitProbe)
        {
            ctx->probesize = 65536;
            // Microseconds of media; must cover a few frames so formats that
            // fill parameters from a decoded frame (mp3) complete under the cap.
            ctx->max_analyze_duration = 200000;
        }

        // avformat_open_input frees the user supplied AVFormatContext on failure
        if (avformat_open_input(&ctx, name, fmt, nullptr) != 0)
            return false;

        formatCtx.reset(std::exchange(ctx, nullptr));

        stream = findAudioStream(*formatCtx);

        // The demuxers for Morrowind's formats fill the codec parameters from the
        // file header during avformat_open_input, so skip avformat_find_stream_info:
        // it scans packets across the whole file for duration and bitrate estimates
        // that nothing here reads, costing several milliseconds per wav on the
        // thread that starts the sound (#4880).
        if (stream == nullptr || !hasCompleteAudioParameters(**stream))
        {
            if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0)
                return false;
            stream = findAudioStream(*formatCtx);
        }

        return stream != nullptr;
    }

    void FFmpegDecoder::open(VFS::Path::NormalizedView fname)
    {
        close();
        bool cached = false;
        if (mHeadCache != nullptr)
        {
            if (std::shared_ptr<const HeadBuffer> buffer = mHeadCache->lookup(fname))
            {
                mDataStream = makeHeadStream(std::move(buffer), *mResourceMgr);
                cached = true;
            }
            else
                mDataStream = makeRecordingStream(mResourceMgr->get(fname));
        }
        else
            mDataStream = mResourceMgr->get(fname);

        AVIOContextPtr ioCtx;
        AVFormatContextPtr formatCtxPtr;
        AVStream** stream = nullptr;

        // Naming the demuxer from the extension lets avformat_open_input skip
        // content probing, and the probe caps bound the scan for formats whose
        // header leaves parameters incomplete (mp3 gets sample rate and channel
        // count from the first decoded frame). Mislabelled files fail this
        // attempt and take the probed path below.
        bool opened = false;
        if (const AVInputFormat* fmt = guessFormat(fname))
        {
            // Lenient demuxers (mp3) can accept mislabelled data and produce a
            // stream with empty parameters; that counts as a failed attempt too.
            opened = openContext(fname.value().data(), fmt, true, ioCtx, formatCtxPtr, stream)
                && hasCompleteAudioParameters(**stream);
            if (!opened)
            {
                mDataStream->clear();
                mDataStream->seekg(0);
            }
        }

        if (!opened && !openContext(fname.value().data(), nullptr, false, ioCtx, formatCtxPtr, stream))
            throw std::runtime_error("Failed to open input");

        // Opening is done, so the bytes it read are exactly the prefix the next open of this file needs.
        if (mHeadCache != nullptr && !cached)
            mHeadCache->insert(fname, *mDataStream);

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

#if OPENMW_FFMPEG_5_OR_GREATER
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

    void FFmpegDecoder::close()
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

    std::string FFmpegDecoder::getName()
    {
// In the FFMpeg 4.0 a "filename" field was replaced by "url"
#if LIBAVCODEC_VERSION_INT < 3805796
        return mFormatCtx->filename;
#else
        return mFormatCtx->url;
#endif
    }

    void FFmpegDecoder::getInfo(int* samplerate, ChannelConfig* chans, SampleType* type)
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

#if OPENMW_FFMPEG_5_OR_GREATER
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
#if OPENMW_FFMPEG_5_OR_GREATER
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
#if OPENMW_FFMPEG_5_OR_GREATER
        AVChannelLayout chLayout = mCodecCtx->ch_layout;
        if (chLayout.u.mask == 0)
            av_channel_layout_default(&chLayout, mCodecCtx->ch_layout.nb_channels);

        if (mOutputSampleFormat != mCodecCtx->sample_fmt || mOutputChannelLayout.u.mask != chLayout.u.mask)
#else
        int64_t ch_layout = mCodecCtx->channel_layout;
        if (ch_layout == 0)
            ch_layout = av_get_default_channel_layout(mCodecCtx->channels);

        if (mOutputSampleFormat != mCodecCtx->sample_fmt || mOutputChannelLayout != ch_layout)
#endif

        {
#if OPENMW_FFMPEG_5_OR_GREATER
            swr_alloc_set_opts2(&mSwr, // SwrContext
                &mOutputChannelLayout, // output ch layout
                mOutputSampleFormat, // output sample format
                mCodecCtx->sample_rate, // output sample rate
                &chLayout, // input ch layout
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

    size_t FFmpegDecoder::read(char* buffer, size_t bytes)
    {
        if (!mStream)
        {
            Log(Debug::Error) << "No audio stream";
            return 0;
        }
        return readAVAudioData(buffer, bytes);
    }

    void FFmpegDecoder::readAll(std::vector<char>& output)
    {
        if (!mStream)
        {
            Log(Debug::Error) << "No audio stream";
            return;
        }

        while (getAVAudioData())
        {
#if OPENMW_FFMPEG_5_OR_GREATER
            size_t got = mFrame->nb_samples * mOutputChannelLayout.nb_channels
#else
            size_t got = mFrame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout)
#endif
                * av_get_bytes_per_sample(mOutputSampleFormat);
            const char* inbuf = reinterpret_cast<char*>(mFrameData[0]);
            output.insert(output.end(), inbuf, inbuf + got);
        }
    }

    size_t FFmpegDecoder::getSampleOffset()
    {
#if OPENMW_FFMPEG_5_OR_GREATER
        std::size_t delay = (mFrameSize - mFramePos) / mOutputChannelLayout.nb_channels
#else
        std::size_t delay = (mFrameSize - mFramePos) / av_get_channel_layout_nb_channels(mOutputChannelLayout)
#endif
            / av_get_bytes_per_sample(mOutputSampleFormat);
        return static_cast<std::size_t>(mNextPts * mCodecCtx->sample_rate) - delay;
    }

    FFmpegDecoder::FFmpegDecoder(const VFS::Manager* vfs, HeadCache* headCache)
        : SoundDecoder(vfs)
        , mStream(nullptr)
        , mFrameSize(0)
        , mFramePos(0)
        , mNextPts(0.0)
        , mSwr(nullptr)
        , mOutputSampleFormat(AV_SAMPLE_FMT_NONE)
#if OPENMW_FFMPEG_5_OR_GREATER
        , mOutputChannelLayout({})
#else
        , mOutputChannelLayout(0)
#endif
        , mDataBuf(nullptr)
        , mFrameData(nullptr)
        , mDataBufLen(0)
        , mHeadCache(headCache)
    {
        memset(&mPacket, 0, sizeof(mPacket));

        /* We need to make sure ffmpeg is initialized. Optionally silence warning
         * output from the lib */
        [[maybe_unused]] static const bool doneInit = [] {
// This is not needed anymore above FFMpeg version 4.0
#if LIBAVCODEC_VERSION_INT < 3805796
            av_register_all();
#endif
            av_log_set_level(AV_LOG_ERROR);
            return true;
        }();
    }

    FFmpegDecoder::~FFmpegDecoder()
    {
        close();
    }
}
