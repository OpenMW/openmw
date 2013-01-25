#ifdef OPENMW_USE_FFMPEG


#include "ffmpeg_decoder.hpp"

// auto_ptr
#include <memory>

#include <stdexcept>

namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("FFmpeg exception: "+msg); }


int FFmpeg_Decoder::readPacket(void *user_data, uint8_t *buf, int buf_size)
{
    Ogre::DataStreamPtr stream = static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;
    return stream->read(buf, buf_size);
}

int FFmpeg_Decoder::writePacket(void *user_data, uint8_t *buf, int buf_size)
{
    Ogre::DataStreamPtr stream = static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;
    return stream->write(buf, buf_size);
}

int64_t FFmpeg_Decoder::seek(void *user_data, int64_t offset, int whence)
{
    Ogre::DataStreamPtr stream = static_cast<FFmpeg_Decoder*>(user_data)->mDataStream;

    whence &= ~AVSEEK_FORCE;
    if(whence == AVSEEK_SIZE)
        return stream->size();
    if(whence == SEEK_SET)
        stream->seek(offset);
    else if(whence == SEEK_CUR)
        stream->seek(stream->tell()+offset);
    else if(whence == SEEK_END)
        stream->seek(stream->size()+offset);
    else
        return -1;

    return stream->tell();
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
            if((uint64_t)mPacket.pts != AV_NOPTS_VALUE)
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
    int got_frame, len;

    if((*mStream)->codec->codec_type != AVMEDIA_TYPE_AUDIO)
        return false;

    do {
        if(mPacket.size == 0 && !getNextPacket())
            return false;

        /* Decode some data, and check for errors */
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
            mFrameSize = mFrame->nb_samples * (*mStream)->codec->channels *
                         av_get_bytes_per_sample((*mStream)->codec->sample_fmt);
        }

        /* Get the amount of bytes remaining to be written, and clamp to
         * the amount of decoded data we have */
        size_t rem = std::min<size_t>(length-dec, mFrameSize-mFramePos);

        /* Copy the data to the app's buffer and increment */
        memcpy(data, mFrame->data[0]+mFramePos, rem);
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
    mDataStream = mResourceMgr.openResource(fname);

    if((mFormatCtx=avformat_alloc_context()) == NULL)
        fail("Failed to allocate context");

    mFormatCtx->pb = avio_alloc_context(NULL, 0, 0, this, readPacket, writePacket, seek);
    if(!mFormatCtx->pb || avformat_open_input(&mFormatCtx, fname.c_str(), NULL, NULL) != 0)
    {
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
        fail("Failed to allocate input stream");
    }

    try
    {
        if(avformat_find_stream_info(mFormatCtx, NULL) < 0)
            fail("Failed to find stream info in "+fname);

        for(size_t j = 0;j < mFormatCtx->nb_streams;j++)
        {
            if(mFormatCtx->streams[j]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                mStream = &mFormatCtx->streams[j];
                break;
            }
        }
        if(!mStream)
            fail("No audio streams in "+fname);

        AVCodec *codec = avcodec_find_decoder((*mStream)->codec->codec_id);
        if(!codec)
        {
            std::stringstream ss("No codec found for id ");
            ss << (*mStream)->codec->codec_id;
            fail(ss.str());
        }
        if(avcodec_open2((*mStream)->codec, codec, NULL) < 0)
            fail("Failed to open audio codec " + std::string(codec->long_name));

        mFrame = avcodec_alloc_frame();
    }
    catch(std::exception &e)
    {
        avformat_close_input(&mFormatCtx);
        throw;
    }
}

void FFmpeg_Decoder::close()
{
    if(mStream)
        avcodec_close((*mStream)->codec);
    mStream = NULL;

    av_free_packet(&mPacket);
    av_freep(&mFrame);

    if(mFormatCtx)
    {
        AVIOContext* context = mFormatCtx->pb;
        avformat_close_input(&mFormatCtx);
        av_free(context);
    }

    mDataStream.setNull();
}

std::string FFmpeg_Decoder::getName()
{
    return mFormatCtx->filename;
}

void FFmpeg_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(!mStream)
        fail("No audio stream info");

    if((*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_U8)
        *type = SampleType_UInt8;
    else if((*mStream)->codec->sample_fmt == AV_SAMPLE_FMT_S16)
        *type = SampleType_Int16;
    else
        fail(std::string("Unsupported sample format: ")+
             av_get_sample_fmt_name((*mStream)->codec->sample_fmt));

    if((*mStream)->codec->channel_layout == AV_CH_LAYOUT_MONO)
        *chans = ChannelConfig_Mono;
    else if((*mStream)->codec->channel_layout == AV_CH_LAYOUT_STEREO)
        *chans = ChannelConfig_Stereo;
    else if((*mStream)->codec->channel_layout == AV_CH_LAYOUT_QUAD)
        *chans = ChannelConfig_Quad;
    else if((*mStream)->codec->channel_layout == AV_CH_LAYOUT_5POINT1)
        *chans = ChannelConfig_5point1;
    else if((*mStream)->codec->channel_layout == AV_CH_LAYOUT_7POINT1)
        *chans = ChannelConfig_7point1;
    else if((*mStream)->codec->channel_layout == 0)
    {
        /* Unknown channel layout. Try to guess. */
        if((*mStream)->codec->channels == 1)
            *chans = ChannelConfig_Mono;
        else if((*mStream)->codec->channels == 2)
            *chans = ChannelConfig_Stereo;
        else
        {
            std::stringstream sstr("Unsupported raw channel count: ");
            sstr << (*mStream)->codec->channels;
            fail(sstr.str());
        }
    }
    else
    {
        char str[1024];
        av_get_channel_layout_string(str, sizeof(str), (*mStream)->codec->channels,
                                     (*mStream)->codec->channel_layout);
        fail(std::string("Unsupported channel layout: ")+str);
    }

    *samplerate = (*mStream)->codec->sample_rate;
}

size_t FFmpeg_Decoder::read(char *buffer, size_t bytes)
{
    if(!mStream)
        fail("No audio stream");
    return readAVAudioData(buffer, bytes);
}

void FFmpeg_Decoder::readAll(std::vector<char> &output)
{
    if(!mStream)
        fail("No audio stream");

    while(getAVAudioData())
    {
        size_t got = mFrame->nb_samples * (*mStream)->codec->channels *
                     av_get_bytes_per_sample((*mStream)->codec->sample_fmt);
        const char *inbuf = reinterpret_cast<char*>(mFrame->data[0]);
        output.insert(output.end(), inbuf, inbuf+got);
    }
}

void FFmpeg_Decoder::rewind()
{
    int stream_idx = mStream - mFormatCtx->streams;
    if(av_seek_frame(mFormatCtx, stream_idx, 0, 0) < 0)
        fail("Failed to seek in audio stream");
    av_free_packet(&mPacket);
    mFrameSize = mFramePos = 0;
    mNextPts = 0.0;
}

size_t FFmpeg_Decoder::getSampleOffset()
{
    int delay = (mFrameSize-mFramePos) / (*mStream)->codec->channels /
                av_get_bytes_per_sample((*mStream)->codec->sample_fmt);
    return (int)(mNextPts*(*mStream)->codec->sample_rate) - delay;
}

FFmpeg_Decoder::FFmpeg_Decoder()
  : mFormatCtx(NULL)
  , mStream(NULL)
  , mFrame(NULL)
  , mFrameSize(0)
  , mFramePos(0)
  , mNextPts(0.0)
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

#endif
