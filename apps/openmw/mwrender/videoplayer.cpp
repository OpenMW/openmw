#include "videoplayer.hpp"


#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwsound/sound_decoder.hpp"
#include "../mwsound/sound.hpp"


#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define AV_SYNC_THRESHOLD 0.01
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20
#define VIDEO_PICTURE_QUEUE_SIZE 1


namespace MWRender
{

enum {
    AV_SYNC_AUDIO_MASTER,
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_MASTER,

    AV_SYNC_DEFAULT = AV_SYNC_EXTERNAL_MASTER
};

struct PacketQueue {
    PacketQueue()
      : first_pkt(NULL), last_pkt(NULL), nb_packets(0), size(0)
    { }
    ~PacketQueue()
    { flush(); }

    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;

    boost::mutex mutex;
    boost::condition_variable cond;

    void put(AVPacket *pkt);
    int get(AVPacket *pkt, VideoState *is);

    void flush();
};

struct VideoPicture {
    VideoPicture() : pts(0.0)
    { }

    std::vector<uint8_t> data;
    double pts;
};

struct VideoState {
    VideoState()
      : videoStream(-1), audioStream(-1), av_sync_type(0), external_clock_base(0),
        audio_clock(0), audio_st(NULL), audio_diff_cum(0), audio_diff_avg_coef(0),
        audio_diff_threshold(0), audio_diff_avg_count(0), frame_last_pts(0),
        frame_last_delay(0), video_clock(0), video_st(NULL), rgbaFrame(NULL), pictq_size(0),
        pictq_rindex(0), pictq_windex(0), quit(false), refresh(0), format_ctx(0),
        sws_context(NULL), display_ready(0)
    { }

    ~VideoState()
    { }

    void init(const std::string& resourceName);
    void deinit();

    int stream_open(int stream_index, AVFormatContext *pFormatCtx);

    static void video_thread_loop(VideoState *is);
    static void decode_thread_loop(VideoState *is);

    void video_display();
    void video_refresh_timer();

    int queue_picture(AVFrame *pFrame, double pts);
    double synchronize_video(AVFrame *src_frame, double pts);

    static void video_refresh(VideoState *is);


    double get_audio_clock()
    { return this->AudioTrack->getTimeOffset(); }

    double get_video_clock()
    { return this->frame_last_pts; }

    double get_external_clock()
    { return ((uint64_t)av_gettime()-this->external_clock_base) / 1000000.0; }

    double get_master_clock()
    {
        if(this->av_sync_type == AV_SYNC_VIDEO_MASTER)
            return this->get_video_clock();
        if(this->av_sync_type == AV_SYNC_AUDIO_MASTER)
            return this->get_audio_clock();
        return this->get_external_clock();
    }


    static int OgreResource_Read(void *user_data, uint8_t *buf, int buf_size);
    static int OgreResource_Write(void *user_data, uint8_t *buf, int buf_size);
    static int64_t OgreResource_Seek(void *user_data, int64_t offset, int whence);


    int videoStream, audioStream;

    int av_sync_type;
    uint64_t external_clock_base;

    double      audio_clock;
    AVStream   *audio_st;
    PacketQueue audioq;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int    audio_diff_avg_count;

    double      frame_last_pts;
    double      frame_last_delay;
    double      video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    AVStream    *video_st;
    PacketQueue videoq;

    Ogre::DataStreamPtr stream;

    MWBase::SoundPtr AudioTrack;

    AVFormatContext* format_ctx;
    SwsContext* sws_context;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
    AVFrame*     rgbaFrame; // used as buffer for the frame converted from its native format to RGBA
    int          pictq_size, pictq_rindex, pictq_windex;

    boost::mutex pictq_mutex;
    boost::condition_variable pictq_cond;

    boost::thread parse_thread;
    boost::thread video_thread;

    boost::thread refresh_thread;
    volatile int refresh_rate_ms;

    volatile bool refresh;
    volatile int quit;

    int display_ready;
};


void PacketQueue::put(AVPacket *pkt)
{
    AVPacketList *pkt1;
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if(!pkt1) throw std::bad_alloc();
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    if(pkt1->pkt.destruct == NULL)
    {
        if(av_dup_packet(&pkt1->pkt) < 0)
        {
            av_free(pkt1);
            throw std::runtime_error("Failed to duplicate packet");
        }
        av_free_packet(pkt);
    }

    this->mutex.lock ();

    if(!last_pkt)
        this->first_pkt = pkt1;
    else
        this->last_pkt->next = pkt1;
    this->last_pkt = pkt1;
    this->nb_packets++;
    this->size += pkt1->pkt.size;
    this->cond.notify_one();

    this->mutex.unlock();
}

int PacketQueue::get(AVPacket *pkt, VideoState *is)
{
    boost::unique_lock<boost::mutex> lock(this->mutex);
    while(!is->quit)
    {
        AVPacketList *pkt1 = this->first_pkt;
        if(pkt1)
        {
            this->first_pkt = pkt1->next;
            if(!this->first_pkt)
                this->last_pkt = NULL;
            this->nb_packets--;
            this->size -= pkt1->pkt.size;

            *pkt = pkt1->pkt;
            av_free(pkt1);

            return 1;
        }

        this->cond.wait(lock);
    }

    return -1;
}

void PacketQueue::flush()
{
    AVPacketList *pkt, *pkt1;

    this->mutex.lock();
    for(pkt = this->first_pkt; pkt != NULL; pkt = pkt1)
    {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    this->last_pkt = NULL;
    this->first_pkt = NULL;
    this->nb_packets = 0;
    this->size = 0;
    this->mutex.unlock ();
}


class MovieAudioDecoder : public MWSound::Sound_Decoder
{
    static void fail(const std::string &str)
    {
        throw std::runtime_error(str);
    }

    struct AutoAVPacket : public AVPacket {
        AutoAVPacket(int size=0)
        {
            if(av_new_packet(this, size) < 0)
                throw std::bad_alloc();
        }
        ~AutoAVPacket()
        { av_free_packet(this); }
    };

    VideoState *is;

    AutoAVPacket mPacket;
    AVFrame *mFrame;
    ssize_t mFramePos;
    ssize_t mFrameSize;

    /* Add or subtract samples to get a better sync, return number of bytes to
     * skip (negative means to duplicate). */
    int synchronize_audio()
    {
        if(is->av_sync_type == AV_SYNC_AUDIO_MASTER)
            return 0;

        int sample_skip = 0;

        // accumulate the clock difference
        double diff = is->get_master_clock() - is->get_audio_clock();
        is->audio_diff_cum = diff + is->audio_diff_avg_coef *
                                    is->audio_diff_cum;
        if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
            is->audio_diff_avg_count++;
        else
        {
            double avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
            if(fabs(avg_diff) >= is->audio_diff_threshold)
            {
                int n = av_get_bytes_per_sample(is->audio_st->codec->sample_fmt) *
                        is->audio_st->codec->channels;
                sample_skip = ((int)(diff * is->audio_st->codec->sample_rate) * n);
            }
        }

        return sample_skip;
    }

    int audio_decode_frame(AVFrame *frame)
    {
        AVPacket *pkt = &mPacket;

        for(;;)
        {
            while(pkt->size > 0)
            {
                int len1, got_frame;

                len1 = avcodec_decode_audio4(is->audio_st->codec, frame, &got_frame, pkt);
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

                is->audio_clock += (double)frame->nb_samples /
                                   (double)is->audio_st->codec->sample_rate;

                /* We have data, return it and come back for more later */
                return frame->nb_samples * av_get_bytes_per_sample(is->audio_st->codec->sample_fmt) *
                       is->audio_st->codec->channels;
            }
            av_free_packet(pkt);

            if(is->quit)
                return -1;

            /* next packet */
            if(is->audioq.get(pkt, is) < 0)
                return -1;

            /* if update, update the audio clock w/pts */
            if((uint64_t)pkt->pts != AV_NOPTS_VALUE)
                is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
        }
    }

    void open(const std::string&)
    { fail(std::string("Invalid call to ")+__PRETTY_FUNCTION__); }

    void close() { }

    std::string getName()
    { return is->stream->getName(); }

    void rewind() { }

public:
    MovieAudioDecoder(VideoState *_is)
      : is(_is)
      , mFrame(avcodec_alloc_frame())
      , mFramePos(0)
      , mFrameSize(0)
    { }
    virtual ~MovieAudioDecoder()
    {
        av_freep(&mFrame);
    }

    void getInfo(int *samplerate, MWSound::ChannelConfig *chans, MWSound::SampleType * type)
    {
        if(is->audio_st->codec->sample_fmt == AV_SAMPLE_FMT_U8)
            *type = MWSound::SampleType_UInt8;
        else if(is->audio_st->codec->sample_fmt == AV_SAMPLE_FMT_S16)
            *type = MWSound::SampleType_Int16;
        else
            fail(std::string("Unsupported sample format: ")+
                 av_get_sample_fmt_name(is->audio_st->codec->sample_fmt));

        if(is->audio_st->codec->channel_layout == AV_CH_LAYOUT_MONO)
            *chans = MWSound::ChannelConfig_Mono;
        else if(is->audio_st->codec->channel_layout == AV_CH_LAYOUT_STEREO)
            *chans = MWSound::ChannelConfig_Stereo;
        else if(is->audio_st->codec->channel_layout == AV_CH_LAYOUT_QUAD)
            *chans = MWSound::ChannelConfig_Quad;
        else if(is->audio_st->codec->channel_layout == AV_CH_LAYOUT_5POINT1)
            *chans = MWSound::ChannelConfig_5point1;
        else if(is->audio_st->codec->channel_layout == AV_CH_LAYOUT_7POINT1)
            *chans = MWSound::ChannelConfig_7point1;
        else if(is->audio_st->codec->channel_layout == 0)
        {
            /* Unknown channel layout. Try to guess. */
            if(is->audio_st->codec->channels == 1)
                *chans = MWSound::ChannelConfig_Mono;
            else if(is->audio_st->codec->channels == 2)
                *chans = MWSound::ChannelConfig_Stereo;
            else
            {
                std::stringstream sstr("Unsupported raw channel count: ");
                sstr << is->audio_st->codec->channels;
                fail(sstr.str());
            }
        }
        else
        {
            char str[1024];
            av_get_channel_layout_string(str, sizeof(str), is->audio_st->codec->channels,
                                         is->audio_st->codec->channel_layout);
            fail(std::string("Unsupported channel layout: ")+str);
        }

        *samplerate = is->audio_st->codec->sample_rate;
    }

    size_t read(char *stream, size_t len)
    {
        int sample_skip = synchronize_audio();
        size_t total = 0;

        while(total < len)
        {
            while(mFramePos >= mFrameSize)
            {
                /* We have already sent all our data; get more */
                mFrameSize = audio_decode_frame(mFrame);
                if(mFrameSize < 0)
                {
                    /* If error, we're done */
                    break;
                }

                mFramePos = std::min(mFrameSize, sample_skip);
                sample_skip -= mFramePos;
            }

            size_t len1 = len - total;
            if(mFramePos >= 0)
            {
                len1 = std::min<size_t>(len1, mFrameSize-mFramePos);
                memcpy(stream, mFrame->data[0]+mFramePos, len1);
            }
            else
            {
                len1 = std::min<size_t>(len1, -mFramePos);

                int n = av_get_bytes_per_sample(is->audio_st->codec->sample_fmt) *
                        is->audio_st->codec->channels;

                /* add samples by copying the first sample*/
                if(n == 1)
                    memset(stream, *mFrame->data[0], len1);
                else if(n == 2)
                {
                    const int16_t val = *((int16_t*)mFrame->data[0]);
                    for(size_t nb = 0;nb < len1;nb += n)
                        *((int16_t*)(stream+nb)) = val;
                }
                else if(n == 4)
                {
                    const int32_t val = *((int32_t*)mFrame->data[0]);
                    for(size_t nb = 0;nb < len1;nb += n)
                        *((int32_t*)(stream+nb)) = val;
                }
                else if(n == 8)
                {
                    const int64_t val = *((int64_t*)mFrame->data[0]);
                    for(size_t nb = 0;nb < len1;nb += n)
                        *((int64_t*)(stream+nb)) = val;
                }
                else
                {
                    for(size_t nb = 0;nb < len1;nb += n)
                        memcpy(stream+nb, mFrame->data[0], n);
                }
            }

            total += len1;
            stream += len1;
            mFramePos += len1;
        }

        return total;
    }

    size_t getSampleOffset()
    {
        return (size_t)(is->audio_clock*is->audio_st->codec->sample_rate);
    }
};


int VideoState::OgreResource_Read(void *user_data, uint8_t *buf, int buf_size)
{
    Ogre::DataStreamPtr stream = static_cast<VideoState*>(user_data)->stream;
    return stream->read(buf, buf_size);
}

int VideoState::OgreResource_Write(void *user_data, uint8_t *buf, int buf_size)
{
    Ogre::DataStreamPtr stream = static_cast<VideoState*>(user_data)->stream;
    return stream->write(buf, buf_size);
}

int64_t VideoState::OgreResource_Seek(void *user_data, int64_t offset, int whence)
{
    Ogre::DataStreamPtr stream = static_cast<VideoState*>(user_data)->stream;

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


void VideoState::video_refresh(VideoState* is)
{
    boost::system_time t = boost::get_system_time();
    while(!is->quit)
    {
        t += boost::posix_time::milliseconds(is->refresh_rate_ms);
        boost::this_thread::sleep(t);
        is->refresh = true;
    }
}


void VideoState::video_display()
{
    VideoPicture *vp = &this->pictq[this->pictq_rindex];

    if(this->video_st->codec->width != 0 && this->video_st->codec->height != 0)
    {
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("VideoTexture");
        if(texture.isNull () || static_cast<int>(texture->getWidth()) != this->video_st->codec->width
                             || static_cast<int>(texture->getHeight()) != this->video_st->codec->height)
        {
            Ogre::TextureManager::getSingleton ().remove ("VideoTexture");
            texture = Ogre::TextureManager::getSingleton().createManual(
                                    "VideoTexture",
                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                    Ogre::TEX_TYPE_2D,
                                    this->video_st->codec->width, this->video_st->codec->height,
                                    0,
                                    Ogre::PF_BYTE_RGBA,
                                    Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
        }
        Ogre::PixelBox pb(this->video_st->codec->width, this->video_st->codec->height, 1, Ogre::PF_BYTE_RGBA, &vp->data[0]);
        Ogre::HardwarePixelBufferSharedPtr buffer = texture->getBuffer();
        buffer->blitFromMemory(pb);
        this->display_ready = 1;
    }
}

void VideoState::video_refresh_timer()
{
    VideoPicture *vp;
    double delay;

    if(this->pictq_size == 0)
        return;

    vp = &this->pictq[this->pictq_rindex];

    delay = vp->pts - this->frame_last_pts; /* the pts from last time */
    if(delay <= 0 || delay >= 1.0) {
        /* if incorrect delay, use previous one */
        delay = this->frame_last_delay;
    }
    /* save for next time */
    this->frame_last_delay = delay;
    this->frame_last_pts = vp->pts;

    /* FIXME: Syncing should be done in the decoding stage, where frames can be
     * skipped or duplicated as needed. */
    /* update delay to sync to audio if not master source */
    if(this->av_sync_type != AV_SYNC_VIDEO_MASTER)
    {
        double diff = this->get_video_clock() - this->get_master_clock();

        /* Skip or repeat the frame. Take delay into account
         * FFPlay still doesn't "know if this is the best guess." */
        double sync_threshold = std::max(delay, AV_SYNC_THRESHOLD);
        if(diff <= -sync_threshold)
            delay = 0;
        else if(diff >= sync_threshold)
            delay = 2 * delay;
    }

    this->refresh_rate_ms = std::max<int>(1, (int)(delay*1000.0));
    /* show the picture! */
    this->video_display();

    /* update queue for next picture! */
    this->pictq_rindex = (this->pictq_rindex+1) % VIDEO_PICTURE_QUEUE_SIZE;
    this->pictq_mutex.lock();
    this->pictq_size--;
    this->pictq_cond.notify_one();
    this->pictq_mutex.unlock();
}


int VideoState::queue_picture(AVFrame *pFrame, double pts)
{
    VideoPicture *vp;

    /* wait until we have a new pic */
    {
        boost::unique_lock<boost::mutex> lock(this->pictq_mutex);
        while(this->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !this->quit)
            this->pictq_cond.timed_wait(lock, boost::posix_time::milliseconds(1));
    }
    if(this->quit)
        return -1;

    // windex is set to 0 initially
    vp = &this->pictq[this->pictq_windex];

    // Convert the image into RGBA format for Ogre
    if(this->sws_context == NULL)
    {
        int w = this->video_st->codec->width;
        int h = this->video_st->codec->height;
        this->sws_context = sws_getContext(w, h, this->video_st->codec->pix_fmt,
                                           w, h, PIX_FMT_RGBA, SWS_BICUBIC,
                                           NULL, NULL, NULL);
        if(this->sws_context == NULL)
            throw std::runtime_error("Cannot initialize the conversion context!\n");
    }

    vp->pts = pts;
    vp->data.resize(this->video_st->codec->width * this->video_st->codec->height * 4);

    uint8_t *dst = &vp->data[0];
    sws_scale(this->sws_context, pFrame->data, pFrame->linesize,
              0, this->video_st->codec->height, &dst, this->rgbaFrame->linesize);

    // now we inform our display thread that we have a pic ready
    this->pictq_windex = (this->pictq_windex+1) % VIDEO_PICTURE_QUEUE_SIZE;
    this->pictq_mutex.lock();
    this->pictq_size++;
    this->pictq_mutex.unlock();

    return 0;
}

double VideoState::synchronize_video(AVFrame *src_frame, double pts)
{
    double frame_delay;

    /* if we have pts, set video clock to it */
    if(pts != 0)
        this->video_clock = pts;
    else
        pts = this->video_clock;

    /* update the video clock */
    frame_delay = av_q2d(this->video_st->codec->time_base);

    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    this->video_clock += frame_delay;

    return pts;
}


/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;
static int our_get_buffer(struct AVCodecContext *c, AVFrame *pic)
{
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = (uint64_t*)av_malloc(sizeof(uint64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}
static void our_release_buffer(struct AVCodecContext *c, AVFrame *pic)
{
    if(pic) av_freep(&pic->opaque);
    avcodec_default_release_buffer(c, pic);
}


void VideoState::video_thread_loop(VideoState *self)
{
    AVPacket pkt1, *packet = &pkt1;
    int frameFinished;
    AVFrame *pFrame;
    double pts;

    pFrame = avcodec_alloc_frame();

    self->rgbaFrame = avcodec_alloc_frame();
    avpicture_alloc((AVPicture*)self->rgbaFrame, PIX_FMT_RGBA, self->video_st->codec->width, self->video_st->codec->height);

    while(self->videoq.get(packet, self) >= 0)
    {
        // Save global pts to be stored in pFrame
        global_video_pkt_pts = packet->pts;
        // Decode video frame
        if(avcodec_decode_video2(self->video_st->codec, pFrame, &frameFinished, packet) < 0)
            throw std::runtime_error("Error decoding video frame");

        pts = 0;
        if((uint64_t)packet->dts != AV_NOPTS_VALUE)
            pts = packet->dts;
        else if(pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)
            pts = *(uint64_t*)pFrame->opaque;
        pts *= av_q2d(self->video_st->time_base);

        av_free_packet(packet);

        // Did we get a video frame?
        if(frameFinished)
        {
            pts = self->synchronize_video(pFrame, pts);
            if(self->queue_picture(pFrame, pts) < 0)
                break;
        }
    }

    av_free(pFrame);

    avpicture_free((AVPicture*)self->rgbaFrame);
    av_free(self->rgbaFrame);
}

void VideoState::decode_thread_loop(VideoState *self)
{
    AVFormatContext *pFormatCtx = self->format_ctx;
    AVPacket pkt1, *packet = &pkt1;

    try
    {
        if(self->videoStream < 0 && self->audioStream < 0)
            throw std::runtime_error("No streams to decode");

        // main decode loop
        while(!self->quit)
        {
            if((self->audioStream >= 0 && self->audioq.size > MAX_AUDIOQ_SIZE) ||
               (self->videoStream >= 0 && self->videoq.size > MAX_VIDEOQ_SIZE))
            {
                boost::this_thread::sleep(boost::posix_time::milliseconds(10));
                continue;
            }

            if(av_read_frame(pFormatCtx, packet) < 0)
                break;

            // Is this a packet from the video stream?
            if(packet->stream_index == self->videoStream)
                self->videoq.put(packet);
            else if(packet->stream_index == self->audioStream)
                self->audioq.put(packet);
            else
                av_free_packet(packet);
        }
        /* all done - wait for it */
        while(!self->quit)
        {
            // EOF reached, all packets processed, we can exit now
            if(self->audioq.nb_packets == 0 && self->videoq.nb_packets == 0)
                break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
    }
    catch(std::runtime_error& e) {
        std::cerr << "An error occured playing the video: " << e.what () << std::endl;
    }
    catch(Ogre::Exception& e) {
        std::cerr << "An error occured playing the video: " << e.getFullDescription () << std::endl;
    }

    self->quit = 1;
}


int VideoState::stream_open(int stream_index, AVFormatContext *pFormatCtx)
{
    MWSound::DecoderPtr decoder;
    AVCodecContext *codecCtx;
    AVCodec *codec;

    if(stream_index < 0 || stream_index >= static_cast<int>(pFormatCtx->nb_streams))
        return -1;

    // Get a pointer to the codec context for the video stream
    codecCtx = pFormatCtx->streams[stream_index]->codec;
    codec = avcodec_find_decoder(codecCtx->codec_id);
    if(!codec || (avcodec_open2(codecCtx, codec, NULL) < 0))
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    switch(codecCtx->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
        this->audioStream = stream_index;
        this->audio_st = pFormatCtx->streams[stream_index];

        /* averaging filter for audio sync */
        this->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
        this->audio_diff_avg_count = 0;
        /* Correct audio only if larger error than this */
        this->audio_diff_threshold = 2.0 * 0.050/* 50 ms */;

        decoder.reset(new MovieAudioDecoder(this));
        this->AudioTrack = MWBase::Environment::get().getSoundManager()->playTrack(decoder);
        if(!this->AudioTrack)
        {
            this->audioStream = -1;
            avcodec_close(this->audio_st->codec);
            this->audio_st = NULL;
            return -1;
        }
        break;

    case AVMEDIA_TYPE_VIDEO:
        this->videoStream = stream_index;
        this->video_st = pFormatCtx->streams[stream_index];

        this->frame_last_delay = 40e-3;

        codecCtx->get_buffer = our_get_buffer;
        codecCtx->release_buffer = our_release_buffer;
        this->video_thread = boost::thread(video_thread_loop, this);
        this->refresh_thread = boost::thread(video_refresh, this);
        break;

    default:
        break;
    }

    return 0;
}

void VideoState::init(const std::string& resourceName)
{
    try
    {
        int video_index = -1;
        int audio_index = -1;
        unsigned int i;

        this->av_sync_type = AV_SYNC_DEFAULT;
        this->videoStream = -1;
        this->audioStream = -1;
        this->refresh_rate_ms = 10;
        this->refresh = false;
        this->quit = 0;

        this->stream = Ogre::ResourceGroupManager::getSingleton().openResource(resourceName);
        if(this->stream.isNull())
            throw std::runtime_error("Failed to open video resource");

        this->format_ctx = avformat_alloc_context();
        this->format_ctx->pb = avio_alloc_context(NULL, 0, 0, this, OgreResource_Read, OgreResource_Write, OgreResource_Seek);
        if(!this->format_ctx->pb)
        {
            avformat_free_context(this->format_ctx);
            throw std::runtime_error("Failed to allocate ioContext ");
        }

        // Open video file
        /// \todo leak here, ffmpeg or valgrind bug ?
        if (avformat_open_input(&this->format_ctx, resourceName.c_str(), NULL, NULL))
        {
            // "Note that a user-supplied AVFormatContext will be freed on failure."
            this->format_ctx = NULL;
            throw std::runtime_error("Failed to open video input");
        }

        // Retrieve stream information
        if(avformat_find_stream_info(this->format_ctx, NULL) < 0)
            throw std::runtime_error("Failed to retrieve stream information");

        // Dump information about file onto standard error
        av_dump_format(this->format_ctx, 0, resourceName.c_str(), 0);

        for(i = 0;i < this->format_ctx->nb_streams;i++)
        {
            if(this->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0)
                video_index = i;
            if(this->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0)
                audio_index = i;
        }

        this->external_clock_base = av_gettime();
        if(audio_index >= 0)
            this->stream_open(audio_index, this->format_ctx);
        if(video_index >= 0)
            this->stream_open(video_index, this->format_ctx);

        this->parse_thread = boost::thread(decode_thread_loop, this);
    }
    catch(std::runtime_error& e)
    {
        this->quit = 1;
        throw;
    }
    catch(Ogre::Exception& e)
    {
        this->quit = 1;
        throw;
    }
}

void VideoState::deinit()
{
    this->audioq.cond.notify_one();
    this->videoq.cond.notify_one();

    this->parse_thread.join();
    this->video_thread.join();
    this->refresh_thread.join();

    if(this->audioStream >= 0)
        avcodec_close(this->audio_st->codec);
    if(this->videoStream >= 0)
        avcodec_close(this->video_st->codec);

    if(this->sws_context)
        sws_freeContext(this->sws_context);

    if(this->format_ctx)
    {
        AVIOContext *ioContext = this->format_ctx->pb;
        avformat_close_input(&this->format_ctx);
        av_free(ioContext);
    }
}


VideoPlayer::VideoPlayer(Ogre::SceneManager* sceneMgr)
    : mState(NULL)
    , mSceneMgr(sceneMgr)
    , mVideoMaterial(NULL)
    , mRectangle(NULL)
    , mNode(NULL)
{
    mVideoMaterial = Ogre::MaterialManager::getSingleton().create("VideoMaterial", "General");
    mVideoMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
    mVideoMaterial->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
    mVideoMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
    if(mVideoMaterial->getTechnique(0)->getPass(0)->getNumTextureUnitStates() == 0)
        mVideoMaterial->getTechnique(0)->getPass(0)->createTextureUnitState()->setTextureName("black.png");
    else
        mVideoMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("black.png");

    mRectangle = new Ogre::Rectangle2D(true);
    mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
    mRectangle->setMaterial("VideoMaterial");
    mRectangle->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY+1);

    // Use infinite AAB to always stay visible
    Ogre::AxisAlignedBox aabInf;
    aabInf.setInfinite();
    mRectangle->setBoundingBox(aabInf);

    // Attach background to the scene
    mNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
    mNode->attachObject(mRectangle);

    mRectangle->setVisible(false);
    mRectangle->setVisibilityFlags(0x1);
}

VideoPlayer::~VideoPlayer()
{
    if(mState)
        close();

    if(mNode)
        mSceneMgr->destroySceneNode(mNode);
    mNode = NULL;

    if(mRectangle)
        delete mRectangle;
    mRectangle = NULL;
}

void VideoPlayer::playVideo(const std::string &resourceName)
{
    // Register all formats and codecs
    av_register_all();

    if(mState)
        close();

    mRectangle->setVisible(true);

    MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Video);

    // Turn off rendering except the GUI
    mSceneMgr->clearSpecialCaseRenderQueues();
    // SCRQM_INCLUDE with RENDER_QUEUE_OVERLAY does not work.
    for(int i = 0;i < Ogre::RENDER_QUEUE_MAX;++i)
    {
        if(i > 0 && i < 96)
            mSceneMgr->addSpecialCaseRenderQueue(i);
    }
    mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

    MWBase::Environment::get().getSoundManager()->pauseAllSounds();

    mState = new VideoState;
    mState->init(resourceName);
}

void VideoPlayer::update ()
{
    if(mState)
    {
        if(mState->quit)
            close();
        else if(mState->refresh)
        {
            mState->refresh = false;
            mState->video_refresh_timer();
            // Would be nice not to do this all the time...
            if(mState->display_ready)
                mVideoMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("VideoTexture");
        }
    }
}

void VideoPlayer::close()
{
    mState->quit = 1;
    mState->deinit();

    delete mState;
    mState = NULL;

    MWBase::Environment::get().getSoundManager()->resumeAllSounds();

    mRectangle->setVisible(false);
    MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Video);

    mSceneMgr->clearSpecialCaseRenderQueues();
    mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
}

bool VideoPlayer::isPlaying ()
{
    return mState != NULL;
}

}
