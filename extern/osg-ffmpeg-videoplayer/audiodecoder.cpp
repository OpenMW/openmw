#include "audiodecoder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

extern "C"
{
    #include <libavcodec/avcodec.h>

    #include <libswresample/swresample.h>
}

#include "videostate.hpp"

namespace
{
    void fail(const std::string &str)
    {
        throw std::runtime_error(str);
    }

    const double AUDIO_DIFF_AVG_NB = 20;
}

namespace Video
{

// Moved to implementation file, so that HAVE_SWRESAMPLE is used at library compile time only
struct AudioResampler
{
    AudioResampler()
        : mSwr(NULL)
    {
    }

    ~AudioResampler()
    {
        swr_free(&mSwr);
    }

    SwrContext* mSwr;
};

MovieAudioDecoder::MovieAudioDecoder(VideoState* videoState)
    : mVideoState(videoState)
    , mAVStream(*videoState->audio_st)
    , mOutputSampleFormat(AV_SAMPLE_FMT_NONE)
    , mOutputChannelLayout(0)
    , mOutputSampleRate(0)
    , mFramePos(0)
    , mFrameSize(0)
    , mAudioClock(0.0)
    , mDataBuf(NULL)
    , mFrameData(NULL)
    , mDataBufLen(0)
    , mFrame(av_frame_alloc())
    , mGetNextPacket(true)
    , mAudioDiffAccum(0.0)
    , mAudioDiffAvgCoef(exp(log(0.01 / AUDIO_DIFF_AVG_NB)))
    /* Correct audio only if larger error than this */
    , mAudioDiffThreshold(2.0 * 0.050/* 50 ms */)
    , mAudioDiffAvgCount(0)
{
    mAudioResampler.reset(new AudioResampler());

    AVCodec *codec = avcodec_find_decoder(mAVStream->codecpar->codec_id);
    if(!codec)
    {
        std::string ss = "No codec found for id " +
                            std::to_string(mAVStream->codecpar->codec_id);
        throw std::runtime_error(ss);
    }

    AVCodecContext *avctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(avctx, mAVStream->codecpar);

// This is not needed anymore above FFMpeg version 4.0
#if LIBAVCODEC_VERSION_INT < 3805796
    av_codec_set_pkt_timebase(avctx, mAVStream->time_base);
#endif

    mAudioContext = avctx;

    if(avcodec_open2(mAudioContext, codec, nullptr) < 0)
        throw std::runtime_error(std::string("Failed to open audio codec ") + codec->long_name);
}

MovieAudioDecoder::~MovieAudioDecoder()
{
    if(mAudioContext)
        avcodec_free_context(&mAudioContext);

    av_freep(&mFrame);
    av_freep(&mDataBuf);
}

void MovieAudioDecoder::setupFormat()
{
    if (mAudioResampler->mSwr)
        return; // already set up

    AVSampleFormat inputSampleFormat = mAudioContext->sample_fmt;

    uint64_t inputChannelLayout = mAudioContext->channel_layout;
    if (inputChannelLayout == 0)
        inputChannelLayout = av_get_default_channel_layout(mAudioContext->channels);

    int inputSampleRate = mAudioContext->sample_rate;

    mOutputSampleRate = inputSampleRate;
    mOutputSampleFormat = inputSampleFormat;
    mOutputChannelLayout = inputChannelLayout;
    adjustAudioSettings(mOutputSampleFormat, mOutputChannelLayout, mOutputSampleRate);

    if (inputSampleFormat != mOutputSampleFormat
            || inputChannelLayout != mOutputChannelLayout
            || inputSampleRate != mOutputSampleRate)
    {
        mAudioResampler->mSwr = swr_alloc_set_opts(mAudioResampler->mSwr,
                          mOutputChannelLayout,
                          mOutputSampleFormat,
                          mOutputSampleRate,
                          inputChannelLayout,
                          inputSampleFormat,
                          inputSampleRate,
                          0,                             // logging level offset
                          NULL);                         // log context
        if(!mAudioResampler->mSwr)
            fail(std::string("Couldn't allocate SwrContext"));
        if(swr_init(mAudioResampler->mSwr) < 0)
            fail(std::string("Couldn't initialize SwrContext"));
    }
}

int MovieAudioDecoder::synchronize_audio()
{
    if(mVideoState->av_sync_type == AV_SYNC_AUDIO_MASTER)
        return 0;

    int sample_skip = 0;

    // accumulate the clock difference
    double diff = mVideoState->get_master_clock() - mVideoState->get_audio_clock();
    mAudioDiffAccum = diff + mAudioDiffAvgCoef * mAudioDiffAccum;
    if(mAudioDiffAvgCount < AUDIO_DIFF_AVG_NB)
        mAudioDiffAvgCount++;
    else
    {
        double avg_diff = mAudioDiffAccum * (1.0 - mAudioDiffAvgCoef);
        if(fabs(avg_diff) >= mAudioDiffThreshold)
        {
            int n = av_get_bytes_per_sample(mOutputSampleFormat) *
                    av_get_channel_layout_nb_channels(mOutputChannelLayout);
            sample_skip = ((int)(diff * mAudioContext->sample_rate) * n);
        }
    }

    return sample_skip;
}

int MovieAudioDecoder::audio_decode_frame(AVFrame *frame, int &sample_skip)
{
    AVPacket *pkt = &mPacket;

    for(;;)
    {
        /* send the packet with the compressed data to the decoder */
        int ret = 0;
        if (mGetNextPacket)
            ret = avcodec_send_packet(mAudioContext, pkt);

        /* read all the output frames (in general there may be any number of them */
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(mAudioContext, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            {
                // EAGAIN means that we need additional packages to decode this frame.
                // AVERROR_EOF means the end of package.
                mGetNextPacket = true;
                break;
            }
            else if (ret < 0)
            {
                // Error encountered. Stop to decode audio stream.
                av_packet_unref(&mPacket);
                mGetNextPacket = true;
                return -1;
            }

            if(frame->nb_samples <= 0)
                continue;

            if(mAudioResampler->mSwr)
            {
                if(!mDataBuf || mDataBufLen < frame->nb_samples)
                {
                    av_freep(&mDataBuf);
                    if(av_samples_alloc(&mDataBuf, nullptr, av_get_channel_layout_nb_channels(mOutputChannelLayout),
                                        frame->nb_samples, mOutputSampleFormat, 0) < 0)
                        break;
                    else
                        mDataBufLen = frame->nb_samples;
                }

                if(swr_convert(mAudioResampler->mSwr, (uint8_t**)&mDataBuf, frame->nb_samples,
                    (const uint8_t**)frame->extended_data, frame->nb_samples) < 0)
                {
                    break;
                }
                mFrameData = &mDataBuf;
            }
            else
                mFrameData = &frame->data[0];

            int result = frame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout) *
                    av_get_bytes_per_sample(mOutputSampleFormat);

            /* We have data, return it and come back for more later */
            mGetNextPacket = false;
            return result;
        }

        av_packet_unref(&mPacket);
        mGetNextPacket = true;

        /* next packet */
        if(mVideoState->audioq.get(pkt, mVideoState) < 0)
            return -1;

        if(pkt->data == mVideoState->mFlushPktData)
        {
            avcodec_flush_buffers(mAudioContext);
            mAudioDiffAccum = 0.0;
            mAudioDiffAvgCount = 0;
            mAudioClock = av_q2d(mAVStream->time_base)*pkt->pts;
            sample_skip = 0;

            if(mVideoState->audioq.get(pkt, mVideoState) < 0)
                return -1;
        }

        /* if update, update the audio clock w/pts */
        if(pkt->pts != AV_NOPTS_VALUE)
            mAudioClock = av_q2d(mAVStream->time_base)*pkt->pts;
    }
}

size_t MovieAudioDecoder::read(char *stream, size_t len)
{
    if (mVideoState->mPaused)
    {
        // fill the buffer with silence
        size_t sampleSize = av_get_bytes_per_sample(mOutputSampleFormat);
        char* data[1];
        data[0] = stream;
        av_samples_set_silence((uint8_t**)data, 0, len/sampleSize, 1, mOutputSampleFormat);
        return len;
    }

    int sample_skip = synchronize_audio();
    size_t total = 0;

    while(total < len)
    {
        if(mFramePos >= mFrameSize)
        {
            /* We have already sent all our data; get more */
            mFrameSize = audio_decode_frame(mFrame, sample_skip);
            if(mFrameSize < 0)
            {
                /* If error, we're done */
                break;
            }

            mFramePos = std::min<ssize_t>(mFrameSize, sample_skip);
            if(sample_skip > 0 || mFrameSize > -sample_skip)
                sample_skip -= mFramePos;
            continue;
        }

        size_t len1 = len - total;
        if(mFramePos >= 0)
        {
            len1 = std::min<size_t>(len1, mFrameSize-mFramePos);
            memcpy(stream, mFrameData[0]+mFramePos, len1);
        }
        else
        {
            len1 = std::min<size_t>(len1, -mFramePos);

            int n = av_get_bytes_per_sample(mOutputSampleFormat)
                    * av_get_channel_layout_nb_channels(mOutputChannelLayout);

            /* add samples by copying the first sample*/
            if(n == 1)
                memset(stream, *mFrameData[0], len1);
            else if(n == 2)
            {
                const int16_t val = *((int16_t*)mFrameData[0]);
                for(size_t nb = 0;nb < len1;nb += n)
                    *((int16_t*)(stream+nb)) = val;
            }
            else if(n == 4)
            {
                const int32_t val = *((int32_t*)mFrameData[0]);
                for(size_t nb = 0;nb < len1;nb += n)
                    *((int32_t*)(stream+nb)) = val;
            }
            else if(n == 8)
            {
                const int64_t val = *((int64_t*)mFrameData[0]);
                for(size_t nb = 0;nb < len1;nb += n)
                    *((int64_t*)(stream+nb)) = val;
            }
            else
            {
                for(size_t nb = 0;nb < len1;nb += n)
                    memcpy(stream+nb, mFrameData[0], n);
            }
        }

        total += len1;
        stream += len1;
        mFramePos += len1;
    }

    return total;
}

double MovieAudioDecoder::getAudioClock()
{
    return mAudioClock;
}

int MovieAudioDecoder::getOutputSampleRate() const
{
    return mOutputSampleRate;
}

uint64_t MovieAudioDecoder::getOutputChannelLayout() const
{
    return mOutputChannelLayout;
}

AVSampleFormat MovieAudioDecoder::getOutputSampleFormat() const
{
    return mOutputSampleFormat;
}

}
