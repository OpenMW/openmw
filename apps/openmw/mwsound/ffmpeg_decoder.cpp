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
bool FFmpeg_Decoder::getNextPacket(int streamidx)
{
    PacketList *packet;

    packet = (PacketList*)av_malloc(sizeof(*packet));
    packet->next = NULL;

next_packet:
    while(av_read_frame(mFormatCtx, &packet->pkt) >= 0)
    {
        std::vector<MyStream*>::iterator iter = mStreams.begin();

        /* Check each stream the user has a handle for, looking for the one
         * this packet belongs to */
        while(iter != mStreams.end())
        {
            if((*iter)->mStreamIdx == packet->pkt.stream_index)
            {
                PacketList **last;

                last = &(*iter)->mPackets;
                while(*last != NULL)
                    last = &(*last)->next;

                *last = packet;
                if((*iter)->mStreamIdx == streamidx)
                    return true;

                packet = (PacketList*)av_malloc(sizeof(*packet));
                packet->next = NULL;
                goto next_packet;
            }
            iter++;
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
    if(!mPackets && !mParent->getNextPacket(mStreamIdx))
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
        memset(&mPackets->pkt.data[remaining], 0, mPackets->pkt.size - remaining);
        mPackets->pkt.size -= len;
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

        for(size_t j = 0;j < mFormatCtx->nb_streams;j++)
        {
            if(mFormatCtx->streams[j]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                std::auto_ptr<MyStream> stream(new MyStream);
                stream->mCodecCtx = mFormatCtx->streams[j]->codec;
                stream->mStreamIdx = j;
                stream->mPackets = NULL;

                AVCodec *codec = avcodec_find_decoder(stream->mCodecCtx->codec_id);
                if(!codec)
                {
                    std::stringstream ss("No codec found for id ");
                    ss << stream->mCodecCtx->codec_id;
                    fail(ss.str());
                }
                if(avcodec_open(stream->mCodecCtx, codec) < 0)
                    fail("Failed to open audio codec " + std::string(codec->long_name));

                stream->mDecodedData = (char*)av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
                stream->mDecodedDataSize = 0;

                stream->mParent = this;
                mStreams.push_back(stream.release());
                break;
            }
        }
        if(mStreams.empty())
            fail("No audio streams in "+fname);
    }
    catch(std::exception &e)
    {
        av_close_input_file(mFormatCtx);
        mFormatCtx = NULL;
        throw;
    }
}

void FFmpeg_Decoder::close()
{
    while(!mStreams.empty())
    {
        MyStream *stream = mStreams.front();

        stream->clearPackets();
        avcodec_close(stream->mCodecCtx);
        av_free(stream->mDecodedData);
        delete stream;

        mStreams.erase(mStreams.begin());
    }
    if(mFormatCtx)
        av_close_input_file(mFormatCtx);
    mFormatCtx = NULL;

    mDataStream.setNull();
}

void FFmpeg_Decoder::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    if(mStreams.empty())
        fail("No audio stream info");

    MyStream *stream = mStreams[0];
    if(stream->mCodecCtx->sample_fmt == AV_SAMPLE_FMT_U8)
        *type = SampleType_UInt8;
    else if(stream->mCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16)
        *type = SampleType_Int16;
    else
        fail(std::string("Unsupported sample format: ")+
             av_get_sample_fmt_name(stream->mCodecCtx->sample_fmt));

    if(stream->mCodecCtx->channel_layout == AV_CH_LAYOUT_MONO)
        *chans = ChannelConfig_Mono;
    else if(stream->mCodecCtx->channel_layout == AV_CH_LAYOUT_STEREO)
        *chans = ChannelConfig_Stereo;
    else if(stream->mCodecCtx->channel_layout == 0)
    {
        /* Unknown channel layout. Try to guess. */
        if(stream->mCodecCtx->channels == 1)
            *chans = ChannelConfig_Mono;
        else if(stream->mCodecCtx->channels == 2)
            *chans = ChannelConfig_Stereo;
        else
        {
            std::stringstream sstr("Unsupported raw channel count: ");
            sstr << stream->mCodecCtx->channels;
            fail(sstr.str());
        }
    }
    else
    {
        char str[1024];
        av_get_channel_layout_string(str, sizeof(str), stream->mCodecCtx->channels,
                                     stream->mCodecCtx->channel_layout);
        fail(std::string("Unsupported channel layout: ")+str);
    }

    *samplerate = stream->mCodecCtx->sample_rate;
}

size_t FFmpeg_Decoder::read(char *buffer, size_t bytes)
{
    if(mStreams.empty())
        fail("No audio streams");

    return mStreams.front()->readAVAudioData(buffer, bytes);
}

void FFmpeg_Decoder::readAll(std::vector<char> &output)
{
    if(mStreams.empty())
        fail("No audio streams");
    MyStream *stream = mStreams.front();
    char *inbuf;
    size_t got;

    while((inbuf=(char*)stream->getAVAudioData(&got)) != NULL && got > 0)
        output.insert(output.end(), inbuf, inbuf+got);
}

void FFmpeg_Decoder::rewind()
{
    av_seek_frame(mFormatCtx, -1, 0, 0);
    std::for_each(mStreams.begin(), mStreams.end(), std::mem_fun(&MyStream::clearPackets));
}

FFmpeg_Decoder::FFmpeg_Decoder() : mFormatCtx(NULL)
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
