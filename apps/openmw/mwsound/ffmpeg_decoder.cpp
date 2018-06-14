#include "ffmpeg_decoder.hpp"

#include <memory>

#include <stdexcept>
#include <iostream>
#include <algorithm>

#include <components/vfs/manager.hpp>

namespace MWSound
{

int FFmpeg_Decoder::readPacket(void *user_data, uint8_t *buf, int buf_size)
{
    try
    {
        std::istream& stream = *static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;
        stream.clear();
        stream.read((char*)buf, buf_size);
        return stream.gcount();
    }
    catch (std::exception& )
    {
        return 0;
    }
}

int FFmpeg_Decoder::writePacket(void *, uint8_t *, int)
{
    std::cerr<< "can't write to read-only stream" <<std::endl;
    return -1;
}

int64_t FFmpeg_Decoder::seek(void *user_data, int64_t offset, int whence)
{
    std::istream& stream = *static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;

    whence &= ~AVSEEK_FORCE;

    stream.clear();

    if(whence == AVSEEK_SIZE)
    {
        size_t prev = stream.tellg();
        stream.seekg(0, std::ios_base::end);
        size_t size = stream.tellg();
        stream.seekg(prev, std::ios_base::beg);
        return size;
    }

    if(whence == SEEK_SET)
        stream.seekg(offset, std::ios_base::beg);
    else if(whence == SEEK_CUR)
        stream.seekg(offset, std::ios_base::cur);
    else if(whence == SEEK_END)
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
    if(!mStream)
        return false;

    int stream_idx = mStream - mFormatCtx->streams;
    while(av_read_frame(mFormatCtx, &mPacket) >= 0)
    {
        /* Check if the packet belongs to this stream */
        if(stream_idx == mPacket.stream_index)
        {
            if(mPacket.pts != (int64_t)AV_NOPTS_VALUE)
                mNextPts = av_q2d((*mStream)->time_base)*mPacket.pts;
            return true;
        }

        /* Free the packet and look for another */
        av_free_packet(&mPacket);
    }

    return false;
}

bool FFmpeg_Decoder::getAVAudioData()
{
    int got_frame;

    if((*mStream)->codec->codec_type != AVMEDIA_TYPE_AUDIO)
        return false;

    do {
        if(mPacket.size == 0 && !getNextPacket())
            return false;

        /* Decode some data, and check for errors */
        int len = 0;
        if((len=avcodec_decode_audio4((*mStream)->codec, mFrame, &got_frame, &mPacket)) < 0)
            return false;

        /* Move the unread data to the front and clear the end bits */
        int remaining = mPacket.size - len;
        if(remaining <= 0)
            av_free_packet(&mPacket);
        else
        {
            memmove(mPacket.data, &mPacket.data[len], remaining);
            av_shrink_packet(&mPacket, remaining);
        }

        if (!got_frame || mFrame->nb_samples == 0)
            continue;

        if(mSwr)
        {
            if(!mDataBuf || mDataBufLen < mFrame->nb_samples)
            {
                av_freep(&mDataBuf);
                if(av_samples_alloc(&mDataBuf, NULL, av_get_channel_layout_nb_channels(mOutputChannelLayout),
                                    mFrame->nb_samples, mOutputSampleFormat, 0) < 0)
                    return false;
                else
                    mDataBufLen = mFrame->nb_samples;
            }

            if(swr_convert(mSwr, (uint8_t**)&mDataBuf, mFrame->nb_samples,
                (const uint8_t**)mFrame->extended_data, mFrame->nb_samples) < 0)
            {
                return false;
            }
            mFrameData = &mDataBuf;
        }
        else
            mFrameData = &mFrame->data[0];

    } while(got_frame == 0 || mFrame->nb_samples == 0);
    mNextPts += (double)mFrame->nb_samples / (double)(*mStream)->codec->sample_rate;

    return true;
}

size_t FFmpeg_Decoder::readAVAudioData(void *data, size_t length)
{
    size_t dec = 0;

    while(dec < length)
    {
        /* If there's no decoded data, find some */
        if(mFramePos >= mFrameSize)
        {
            if(!getAVAudioData())
                break;
            mFramePos = 0;
            mFrameSize = mFrame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout) *
                         av_get_bytes_per_sample(mOutputSampleFormat);
        }

        /* Get the amount of bytes remaining to be written, and clamp to
         * the amount of decoded data we have */
        size_t rem = std::min<size_t>(length-dec, mFrameSize-mFramePos);

        /* Copy the data to the app's buffer and increment */
        memcpy(data, mFrameData[0]+mFramePos, rem);
        data = (char*)data + rem;
        dec += rem;
        mFramePos += rem;
    }

    /* Return the number of bytes we were able to get */
    return dec;
}

void FFmpeg_Decoder::open(const std::string &fname)
{
    close();
    mDataStream = mResourceMgr->get(fname);

    if((mFormatCtx=avformat_alloc_context()) == NULL)
        throw std::runtime_error("Failed to allocate context");

    try
    {
        mFormatCtx->pb = avio_alloc_context(NULL, 0, 0, this, readPacket, writePacket, seek);
        if(!mFormatCtx->pb || avformat_open_input(&mFormatCtx, fname.c_str(), NULL, NULL) != 0)
        {
            // "Note that a user-supplied AVFormatContext will be freed on failure".
            if (mFormatCtx)
            {
                if (mFormatCtx->pb != NULL)
                {
                    if (mFormatCtx->pb->buffer != NULL)
                    {
                        av_free(mFormatCtx->pb->buffer);
                        mFormatCtx->pb->buffer = NULL;
                    }
                    av_free(mFormatCtx->pb);
                    mFormatCtx->pb = NULL;
                }
                avformat_free_context(mFormatCtx);
            }
            mFormatCtx = NULL;
            throw std::runtime_error("Failed to allocate input stream");
        }

        if(avformat_find_stream_info(mFormatCtx, NULL) < 0)
            throw std::runtime_error("Failed to find stream info in "+fname);

        for(size_t j = 0;j < mFormatCtx->nb_streams;j++)
        {
            if(mFormatCtx->streams[j]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                mStream = &mFormatCtx->streams[j];
                break;
            }
        }
        if(!mStream)
            throw std::runtime_error("No audio streams in "+fname);

        (*mStream)->codec->request_sample_fmt = (*mStream)->codec->sample_fmt;

        AVCodec *codec = avcodec_find_decoder((*mStream)->codec->codec_id);
        if(!codec)
        {
            std::string ss = "No codec found for id " +
                             std::to_string((*mStream)->codec->codec_id);
            throw std::runtime_error(ss);
        }
        if(avcodec_open2((*mStream)->codec, codec, NULL) < 0)
            throw std::runtime_error(std::string("Failed to open audio codec ") +
                                     codec->long_name);

        mFrame = av_frame_alloc();

        if((*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_FLT ||
           (*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_FLTP)
            mOutputSampleFormat = AV_SAMPLE_FMT_S16; // FIXME: Check for AL_EXT_FLOAT32 support
        else if((*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_U8P)
            mOutputSampleFormat = AV_SAMPLE_FMT_U8;
        else if((*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_S16P)
            mOutputSampleFormat = AV_SAMPLE_FMT_S16;
        else
            mOutputSampleFormat = AV_SAMPLE_FMT_S16;

        mOutputChannelLayout = (*mStream)->codec->channel_layout;
        if(mOutputChannelLayout == 0)
            mOutputChannelLayout = av_get_default_channel_layout((*mStream)->codec->channels);
    }
    catch(...)
    {
        if(mStream)
            avcodec_close((*mStream)->codec);
        mStream = NULL;

        if (mFormatCtx != NULL)
        {
            if (mFormatCtx->pb->buffer != NULL)
            {
                av_free(mFormatCtx->pb->buffer);
                mFormatCtx->pb->buffer = NULL;
            }
            av_free(mFormatCtx->pb);
            mFormatCtx->pb = NULL;

            avformat_close_input(&mFormatCtx);
        }
    }
}

void FFmpeg_Decoder::close()
{
    if(mStream)
        avcodec_close((*mStream)->codec);
    mStream = NULL;

    av_free_packet(&mPacket);
    av_freep(&mFrame);
    swr_free(&mSwr);
    av_freep(&mDataBuf);

    if(mFormatCtx)
    {
        if (mFormatCtx->pb != NULL)
        {
            // mFormatCtx->pb->buffer must be freed by hand,
            // if not, valgrind will show memleak, see:
            //
            // https://trac.ffmpeg.org/ticket/1357
            //
            if (mFormatCtx->pb->buffer != NULL)
            {
                av_free(mFormatCtx->pb->buffer);
                mFormatCtx->pb->buffer = NULL;
            }
            av_free(mFormatCtx->pb);
            mFormatCtx->pb = NULL;
        }
        avformat_close_input(&mFormatCtx);
    }

    mDataStream.reset();
}

std::string FFmpeg_Decoder::getName()
{
    return mFormatCtx->filename;
}

void FFmpeg_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(!mStream)
        throw std::runtime_error("No audio stream info");

    if(mOutputSampleFormat == AV_SAMPLE_FMT_U8)
        *type = SampleType_UInt8;
    else if(mOutputSampleFormat == AV_SAMPLE_FMT_S16)
        *type = SampleType_Int16;
    else if(mOutputSampleFormat == AV_SAMPLE_FMT_FLT)
        *type = SampleType_Float32;
    else
    {
        mOutputSampleFormat = AV_SAMPLE_FMT_S16;
        *type = SampleType_Int16;
    }

    if(mOutputChannelLayout == AV_CH_LAYOUT_MONO)
        *chans = ChannelConfig_Mono;
    else if(mOutputChannelLayout == AV_CH_LAYOUT_STEREO)
        *chans = ChannelConfig_Stereo;
    else if(mOutputChannelLayout == AV_CH_LAYOUT_QUAD)
        *chans = ChannelConfig_Quad;
    else if(mOutputChannelLayout == AV_CH_LAYOUT_5POINT1)
        *chans = ChannelConfig_5point1;
    else if(mOutputChannelLayout == AV_CH_LAYOUT_7POINT1)
        *chans = ChannelConfig_7point1;
    else
    {
        char str[1024];
        av_get_channel_layout_string(str, sizeof(str), (*mStream)->codec->channels,
                                     (*mStream)->codec->channel_layout);
        std::cerr<< "Unsupported channel layout: "<<str <<std::endl;

        if((*mStream)->codec->channels == 1)
        {
            mOutputChannelLayout = AV_CH_LAYOUT_MONO;
            *chans = ChannelConfig_Mono;
        }
        else
        {
            mOutputChannelLayout = AV_CH_LAYOUT_STEREO;
            *chans = ChannelConfig_Stereo;
        }
    }

    *samplerate = (*mStream)->codec->sample_rate;
    int64_t ch_layout = (*mStream)->codec->channel_layout;
    if(ch_layout == 0)
        ch_layout = av_get_default_channel_layout((*mStream)->codec->channels);

    if(mOutputSampleFormat != (*mStream)->codec->sample_fmt ||
       mOutputChannelLayout != ch_layout)
    {
        mSwr = swr_alloc_set_opts(mSwr,                   // SwrContext
                          mOutputChannelLayout,           // output ch layout
                          mOutputSampleFormat,            // output sample format
                          (*mStream)->codec->sample_rate, // output sample rate
                          ch_layout,                      // input ch layout
                          (*mStream)->codec->sample_fmt,  // input sample format
                          (*mStream)->codec->sample_rate, // input sample rate
                          0,                              // logging level offset
                          NULL);                          // log context
        if(!mSwr)
            throw std::runtime_error("Couldn't allocate SwrContext");
        if(swr_init(mSwr) < 0)
            throw std::runtime_error("Couldn't initialize SwrContext");
    }
}

size_t FFmpeg_Decoder::read(char *buffer, size_t bytes)
{
    if(!mStream)
    {
        std::cerr<< "No audio stream" <<std::endl;
        return 0;
    }
    return readAVAudioData(buffer, bytes);
}

void FFmpeg_Decoder::readAll(std::vector<char> &output)
{
    if(!mStream)
    {
        std::cerr<< "No audio stream" <<std::endl;
        return;
    }

    while(getAVAudioData())
    {
        size_t got = mFrame->nb_samples * av_get_channel_layout_nb_channels(mOutputChannelLayout) *
                     av_get_bytes_per_sample(mOutputSampleFormat);
        const char *inbuf = reinterpret_cast<char*>(mFrameData[0]);
        output.insert(output.end(), inbuf, inbuf+got);
    }
}

size_t FFmpeg_Decoder::getSampleOffset()
{
    int delay = (mFrameSize-mFramePos) / av_get_channel_layout_nb_channels(mOutputChannelLayout) /
                av_get_bytes_per_sample(mOutputSampleFormat);
    return (int)(mNextPts*(*mStream)->codec->sample_rate) - delay;
}

FFmpeg_Decoder::FFmpeg_Decoder(const VFS::Manager* vfs)
  : Sound_Decoder(vfs)
  , mFormatCtx(NULL)
  , mStream(NULL)
  , mFrame(NULL)
  , mFrameSize(0)
  , mFramePos(0)
  , mNextPts(0.0)
  , mSwr(0)
  , mOutputSampleFormat(AV_SAMPLE_FMT_NONE)
  , mOutputChannelLayout(0)
  , mDataBuf(NULL)
  , mFrameData(NULL)
  , mDataBufLen(0)
{
    memset(&mPacket, 0, sizeof(mPacket));

    /* We need to make sure ffmpeg is initialized. Optionally silence warning
     * output from the lib */
    static bool done_init = false;
    if(!done_init)
    {
        av_register_all();
        av_log_set_level(AV_LOG_ERROR);
        done_init = true;
    }
}

FFmpeg_Decoder::~FFmpeg_Decoder()
{
    close();
}

}
