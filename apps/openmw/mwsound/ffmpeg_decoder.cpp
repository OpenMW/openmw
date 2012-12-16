#ifdef OPENMW_USE_FFMPEG


#include "ffmpeg_decoder.hpp"

// auto_ptr
#include <memory>

#include <stdexcept>

namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("FFmpeg exception: "+msg); }


struct PacketList {
    AVPacket pkt;
    PacketList *next;
};

struct FFmpeg_Decoder::MyStream {
    AVCodecContext *mCodecCtx;
    int mStreamIdx;

    PacketList *mPackets;

    char *mDecodedData;
    size_t mDecodedDataSize;

    FFmpeg_Decoder *mParent;

    void clearPackets();
    void *getAVAudioData(size_t *length);
    size_t readAVAudioData(void *data, size_t length);
};


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
    if(!mStream.get())
        return false;

    PacketList *packet = (PacketList*)av_malloc(sizeof(*packet));
    packet->next = NULL;
    while(av_read_frame(mFormatCtx, &packet->pkt) >= 0)
    {
        /* Check if the packet belongs to this stream */
        if(mStream->mStreamIdx == packet->pkt.stream_index)
        {
            PacketList **last;

            last = &mStream->mPackets;
            while(*last != NULL)
                last = &(*last)->next;

            *last = packet;
            return true;
        }
        /* Free the packet and look for another */
        av_free_packet(&packet->pkt);
    }
    av_free(packet);

    return false;
}

void FFmpeg_Decoder::MyStream::clearPackets()
{
    while(mPackets)
    {
        PacketList *self = mPackets;
        mPackets = self->next;

        av_free_packet(&self->pkt);
        av_free(self);
    }
}

void *FFmpeg_Decoder::MyStream::getAVAudioData(size_t *length)
{
    int size;
    int len;

    if(length) *length = 0;
    if(mCodecCtx->codec_type != AVMEDIA_TYPE_AUDIO)
        return NULL;

    mDecodedDataSize = 0;

next_packet:
    if(!mPackets && !mParent->getNextPacket())
        return NULL;

    /* Decode some data, and check for errors */
    size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    while((len=avcodec_decode_audio3(mCodecCtx, (int16_t*)mDecodedData, &size,
                                     &mPackets->pkt)) == 0)
    {
        PacketList *self;

        if(size > 0)
            break;

        /* Packet went unread and no data was given? Drop it and try the next,
         * I guess... */
        self = mPackets;
        mPackets = self->next;

        av_free_packet(&self->pkt);
        av_free(self);

        if(!mPackets)
            goto next_packet;

        size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    }

    if(len < 0)
        return NULL;

    if(len < mPackets->pkt.size)
    {
        /* Move the unread data to the front and clear the end bits */
        int remaining = mPackets->pkt.size - len;
        memmove(mPackets->pkt.data, &mPackets->pkt.data[len], remaining);
        av_shrink_packet(&mPackets->pkt, remaining);
    }
    else
    {
        PacketList *self;

        self = mPackets;
        mPackets = self->next;

        av_free_packet(&self->pkt);
        av_free(self);
    }

    if(size == 0)
        goto next_packet;

    /* Set the output buffer size */
    mDecodedDataSize = size;
    if(length) *length = mDecodedDataSize;

    return mDecodedData;
}

size_t FFmpeg_Decoder::MyStream::readAVAudioData(void *data, size_t length)
{
    size_t dec = 0;

    while(dec < length)
    {
        /* If there's no decoded data, find some */
        if(mDecodedDataSize == 0)
        {
            if(getAVAudioData(NULL) == NULL)
                break;
        }

        if(mDecodedDataSize > 0)
        {
            /* Get the amount of bytes remaining to be written, and clamp to
             * the amount of decoded data we have */
            size_t rem = length-dec;
            if(rem > mDecodedDataSize)
                rem = mDecodedDataSize;

            /* Copy the data to the app's buffer and increment */
            if(data != NULL)
            {
                memcpy(data, mDecodedData, rem);
                data = (char*)data + rem;
            }
            dec += rem;

            /* If there's any decoded data left, move it to the front of the
             * buffer for next time */
            if(rem < mDecodedDataSize)
                memmove(mDecodedData, &mDecodedData[rem], mDecodedDataSize - rem);
            mDecodedDataSize -= rem;
        }
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

        int audio_idx = -1;
        for(size_t j = 0;j < mFormatCtx->nb_streams;j++)
        {
            if(mFormatCtx->streams[j]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                audio_idx = j;
                break;
            }
        }
        if(audio_idx == -1)
            fail("No audio streams in "+fname);

        std::auto_ptr<MyStream> stream(new MyStream);
        stream->mCodecCtx = mFormatCtx->streams[audio_idx]->codec;
        stream->mStreamIdx = audio_idx;
        stream->mPackets = NULL;

        AVCodec *codec = avcodec_find_decoder(stream->mCodecCtx->codec_id);
        if(!codec)
        {
            std::stringstream ss("No codec found for id ");
            ss << stream->mCodecCtx->codec_id;
            fail(ss.str());
        }
        if(avcodec_open2(stream->mCodecCtx, codec, NULL) < 0)
            fail("Failed to open audio codec " + std::string(codec->long_name));

        stream->mDecodedData = (char*)av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
        stream->mDecodedDataSize = 0;

        stream->mParent = this;
        mStream = stream;
    }
    catch(std::exception &e)
    {
        avformat_close_input(&mFormatCtx);
        mFormatCtx = NULL;
        throw;
    }
}

void FFmpeg_Decoder::close()
{
    if(mStream.get())
    {
        mStream->clearPackets();
        avcodec_close(mStream->mCodecCtx);
        av_free(mStream->mDecodedData);
    }
    mStream.reset();

    if(mFormatCtx)
    {
        AVIOContext* context = mFormatCtx->pb;
        avformat_close_input(&mFormatCtx);
        av_free(context);
    }
    mFormatCtx = NULL;

    mDataStream.setNull();
}

std::string FFmpeg_Decoder::getName()
{
    return mFormatCtx->filename;
}

void FFmpeg_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(!mStream.get())
        fail("No audio stream info");

    if(mStream->mCodecCtx->sample_fmt == AV_SAMPLE_FMT_U8)
        *type = SampleType_UInt8;
    else if(mStream->mCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16)
        *type = SampleType_Int16;
    else
        fail(std::string("Unsupported sample format: ")+
             av_get_sample_fmt_name(mStream->mCodecCtx->sample_fmt));

    if(mStream->mCodecCtx->channel_layout == AV_CH_LAYOUT_MONO)
        *chans = ChannelConfig_Mono;
    else if(mStream->mCodecCtx->channel_layout == AV_CH_LAYOUT_STEREO)
        *chans = ChannelConfig_Stereo;
    else if(mStream->mCodecCtx->channel_layout == AV_CH_LAYOUT_QUAD)
        *chans = ChannelConfig_Quad;
    else if(mStream->mCodecCtx->channel_layout == AV_CH_LAYOUT_5POINT1)
        *chans = ChannelConfig_5point1;
    else if(mStream->mCodecCtx->channel_layout == AV_CH_LAYOUT_7POINT1)
        *chans = ChannelConfig_7point1;
    else if(mStream->mCodecCtx->channel_layout == 0)
    {
        /* Unknown channel layout. Try to guess. */
        if(mStream->mCodecCtx->channels == 1)
            *chans = ChannelConfig_Mono;
        else if(mStream->mCodecCtx->channels == 2)
            *chans = ChannelConfig_Stereo;
        else
        {
            std::stringstream sstr("Unsupported raw channel count: ");
            sstr << mStream->mCodecCtx->channels;
            fail(sstr.str());
        }
    }
    else
    {
        char str[1024];
        av_get_channel_layout_string(str, sizeof(str), mStream->mCodecCtx->channels,
                                     mStream->mCodecCtx->channel_layout);
        fail(std::string("Unsupported channel layout: ")+str);
    }

    *samplerate = mStream->mCodecCtx->sample_rate;
}

size_t FFmpeg_Decoder::read(char *buffer, size_t bytes)
{
    if(!mStream.get())
        fail("No audio stream");

    size_t got = mStream->readAVAudioData(buffer, bytes);
    mSamplesRead += got / mStream->mCodecCtx->channels /
                    av_get_bytes_per_sample(mStream->mCodecCtx->sample_fmt);
    return got;
}

void FFmpeg_Decoder::readAll(std::vector<char> &output)
{
    if(!mStream.get())
        fail("No audio stream");

    char *inbuf;
    size_t got;
    while((inbuf=(char*)mStream->getAVAudioData(&got)) != NULL && got > 0)
    {
        output.insert(output.end(), inbuf, inbuf+got);
        mSamplesRead += got / mStream->mCodecCtx->channels /
                        av_get_bytes_per_sample(mStream->mCodecCtx->sample_fmt);
    }
}

void FFmpeg_Decoder::rewind()
{
    av_seek_frame(mFormatCtx, -1, 0, 0);
    if(mStream.get())
        mStream->clearPackets();
    mSamplesRead = 0;
}

size_t FFmpeg_Decoder::getSampleOffset()
{
    return mSamplesRead;
}

FFmpeg_Decoder::FFmpeg_Decoder() : mFormatCtx(NULL), mSamplesRead(0)
{
    static bool done_init = false;

    /* We need to make sure ffmpeg is initialized. Optionally silence warning
     * output from the lib */
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
