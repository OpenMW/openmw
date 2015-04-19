#include "audiodecoder.hpp"


extern "C"
{

    #include <libavcodec/avcodec.h>

    #ifdef HAVE_LIBSWRESAMPLE
    #include <libswresample/swresample.h>
    #else
    // FIXME: remove this section once libswresample is packaged for Debian
    #include <libavresample/avresample.h>
    #include <libavutil/opt.h>
    #define SwrContext AVAudioResampleContext
    int  swr_init(AVAudioResampleContext *avr);
    void  swr_free(AVAudioResampleContext **avr);
    int swr_convert( AVAudioResampleContext *avr, uint8_t** output, int out_samples, const uint8_t** input, int in_samples);
    AVAudioResampleContext * swr_alloc_set_opts( AVAudioResampleContext *avr, int64_t out_ch_layout, AVSampleFormat out_fmt, int out_rate, int64_t in_ch_layout, AVSampleFormat in_fmt, int in_rate, int o, void* l);
    #endif

    #if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
    #define av_frame_alloc  avcodec_alloc_frame
    #endif


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
    , mFrame(av_frame_alloc())
    , mFramePos(0)
    , mFrameSize(0)
    , mAudioClock(0.0)
    , mAudioDiffAccum(0.0)
    , mAudioDiffAvgCoef(exp(log(0.01 / AUDIO_DIFF_AVG_NB)))
    /* Correct audio only if larger error than this */
    , mAudioDiffThreshold(2.0 * 0.050/* 50 ms */)
    , mAudioDiffAvgCount(0)
    , mOutputSampleFormat(AV_SAMPLE_FMT_NONE)
    , mOutputSampleRate(0)
    , mOutputChannelLayout(0)
    , mDataBuf(NULL)
    , mFrameData(NULL)
    , mDataBufLen(0)
{
    mAudioResampler.reset(new AudioResampler());
}

MovieAudioDecoder::~MovieAudioDecoder()
{
    av_freep(&mFrame);
    av_freep(&mDataBuf);
}

void MovieAudioDecoder::setupFormat()
{
    if (mAudioResampler->mSwr)
        return; // already set up

    AVSampleFormat inputSampleFormat = mAVStream->codec->sample_fmt;

    uint64_t inputChannelLayout = mAVStream->codec->channel_layout;
    if (inputChannelLayout == 0)
        inputChannelLayout = av_get_default_channel_layout(mAVStream->codec->channels);

    int inputSampleRate = mAVStream->codec->sample_rate;

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
            sample_skip = ((int)(diff * mAVStream->codec->sample_rate) * n);
        }
    }

    return sample_skip;
}

int MovieAudioDecoder::audio_decode_frame(AVFrame *frame, int &sample_skip)
{
    AVPacket *pkt = &mPacket;

    for(;;)
    {
        while(pkt->size > 0)
        {
            int len1, got_frame;

            len1 = avcodec_decode_audio4(mAVStream->codec, frame, &got_frame, pkt);
            if(len1 < 0) break;

            if(len1 <= pkt->size)
            {
                /* Move the unread data to the front and clear the end bits */
                int remaining = pkt->size - len1;
                memmove(pkt->data, &pkt->data[len1], remaining);
                av_shrink_packet(pkt, remaining);
            }

            /* No data yet? Look for more frames */
            if(!got_frame || frame->nb_samples <= 0)
                continue;

            if(mAudioResampler->mSwr)
            {
                if(!mDataBuf || mDataBufLen < frame->nb_samples)
                {
                    av_freep(&mDataBuf);
                    if(av_samples_alloc(&mDataBuf, NULL, av_get_channel_layout_nb_channels(mOutputChannelLayout),
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

            mAudioClock += (double)frame->nb_samples /
                           (double)mAVStream->codec->sample_rate;

            /* We have data, return it and come back for more later */
            return frame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout) *
                   av_get_bytes_per_sample(mOutputSampleFormat);
        }
        av_free_packet(pkt);

        /* next packet */
        if(mVideoState->audioq.get(pkt, mVideoState) < 0)
            return -1;

        if(pkt->data == mVideoState->mFlushPktData)
        {
            avcodec_flush_buffers(mAVStream->codec);
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
