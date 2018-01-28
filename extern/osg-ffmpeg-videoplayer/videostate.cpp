#include "videostate.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

#include <osg/Texture2D>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>

    // From libavformat version 55.0.100 and onward the declaration of av_gettime() is
    // removed from libavformat/avformat.h and moved to libavutil/time.h
    // https://github.com/FFmpeg/FFmpeg/commit/06a83505992d5f49846c18507a6c3eb8a47c650e
    #if AV_VERSION_INT(55, 0, 100) <= AV_VERSION_INT(LIBAVFORMAT_VERSION_MAJOR, \
        LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO)
        #include <libavutil/time.h>
    #endif

    #if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
    #define av_frame_alloc  avcodec_alloc_frame
    #endif

}

static const char* flushString = "FLUSH";
struct FlushPacket : AVPacket
{
    FlushPacket()
        : AVPacket()
    {
        data = ( (uint8_t*)flushString);
    }
};

static FlushPacket flush_pkt;

#include "videoplayer.hpp"
#include "audiodecoder.hpp"
#include "audiofactory.hpp"

namespace
{
    const int MAX_AUDIOQ_SIZE = (5 * 16 * 1024);
    const int MAX_VIDEOQ_SIZE = (5 * 256 * 1024);
}

namespace Video
{

VideoState::VideoState()
    : mAudioFactory(NULL)
    , format_ctx(NULL)
    , av_sync_type(AV_SYNC_DEFAULT)
    , audio_st(NULL)
    , video_st(NULL), frame_last_pts(0.0)
    , video_clock(0.0), sws_context(NULL), rgbaFrame(NULL), pictq_size(0)
    , pictq_rindex(0), pictq_windex(0)
    , mSeekRequested(false)
    , mSeekPos(0)
    , mVideoEnded(false)
    , mPaused(false)
    , mQuit(false)
{
    mFlushPktData = flush_pkt.data;

    // Register all formats and codecs
    av_register_all();
}

VideoState::~VideoState()
{
    deinit();
}

void VideoState::setAudioFactory(MovieAudioFactory *factory)
{
    mAudioFactory = factory;
}


void PacketQueue::put(AVPacket *pkt)
{
    AVPacketList *pkt1;
    if(pkt != &flush_pkt && !pkt->buf && av_dup_packet(pkt) < 0)
        throw std::runtime_error("Failed to duplicate packet");

    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if(!pkt1) throw std::bad_alloc();
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    this->mutex.lock ();

    if(!last_pkt)
        this->first_pkt = pkt1;
    else
        this->last_pkt->next = pkt1;
    this->last_pkt = pkt1;
    this->nb_packets++;
    this->size += pkt1->pkt.size;
    this->cond.signal();

    this->mutex.unlock();
}

int PacketQueue::get(AVPacket *pkt, VideoState *is)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(this->mutex);
    while(!is->mQuit)
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

        if(this->flushing)
            break;
        this->cond.wait(&this->mutex);
    }

    return -1;
}

void PacketQueue::flush()
{
    this->flushing = true;
    this->cond.signal();
}

void PacketQueue::clear()
{
    AVPacketList *pkt, *pkt1;

    this->mutex.lock();
    for(pkt = this->first_pkt; pkt != NULL; pkt = pkt1)
    {
        pkt1 = pkt->next;
        if (pkt->pkt.data != flush_pkt.data)
            av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    this->last_pkt = NULL;
    this->first_pkt = NULL;
    this->nb_packets = 0;
    this->size = 0;
    this->mutex.unlock ();
}

int VideoState::istream_read(void *user_data, uint8_t *buf, int buf_size)
{
    try
    {
        std::istream& stream = *static_cast<VideoState*>(user_data)->stream;
        stream.clear();
        stream.read((char*)buf, buf_size);
        return stream.gcount();
    }
    catch (std::exception& )
    {
        return 0;
    }
}

int VideoState::istream_write(void *, uint8_t *, int)
{
    throw std::runtime_error("can't write to read-only stream");
}

int64_t VideoState::istream_seek(void *user_data, int64_t offset, int whence)
{
    std::istream& stream = *static_cast<VideoState*>(user_data)->stream;

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

void VideoState::video_display(VideoPicture *vp)
{
    if((*this->video_st)->codec->width != 0 && (*this->video_st)->codec->height != 0)
    {
        if (!mTexture.get())
        {
            mTexture = new osg::Texture2D;
            mTexture->setDataVariance(osg::Object::DYNAMIC);
            mTexture->setResizeNonPowerOfTwoHint(false);
            mTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            mTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        }

        osg::ref_ptr<osg::Image> image = new osg::Image;

        image->setImage((*this->video_st)->codec->width, (*this->video_st)->codec->height,
                        1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, &vp->data[0], osg::Image::NO_DELETE);

        mTexture->setImage(image);
    }
}

void VideoState::video_refresh()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(this->pictq_mutex);
    if(this->pictq_size == 0)
        return;

    if (this->av_sync_type == AV_SYNC_VIDEO_MASTER)
    {
        VideoPicture* vp = &this->pictq[this->pictq_rindex];
        this->video_display(vp);

        this->pictq_rindex = (pictq_rindex+1) % VIDEO_PICTURE_ARRAY_SIZE;
        this->frame_last_pts = vp->pts;
        this->pictq_size--;
        this->pictq_cond.signal();
    }
    else
    {
        const float threshold = 0.03f;
        if (this->pictq[pictq_rindex].pts > this->get_master_clock() + threshold)
            return; // not ready yet to show this picture

        // TODO: the conversion to RGBA is done in the decoding thread, so if a picture is skipped here, then it was
        // unnecessarily converted. But we may want to replace the conversion by a pixel shader anyway (see comment in queue_picture)
        int i=0;
        for (; i<this->pictq_size-1; ++i)
        {
            if (this->pictq[pictq_rindex].pts + threshold <= this->get_master_clock())
                this->pictq_rindex = (this->pictq_rindex+1) % VIDEO_PICTURE_ARRAY_SIZE; // not enough time to show this picture
            else
                break;
        }

        assert (this->pictq_rindex < VIDEO_PICTURE_ARRAY_SIZE);
        VideoPicture* vp = &this->pictq[this->pictq_rindex];

        this->video_display(vp);

        this->frame_last_pts = vp->pts;

        this->pictq_size -= i;
        // update queue for next picture
        this->pictq_size--;
        this->pictq_rindex = (this->pictq_rindex+1) % VIDEO_PICTURE_ARRAY_SIZE;
        this->pictq_cond.signal();
    }
}


int VideoState::queue_picture(AVFrame *pFrame, double pts)
{
    VideoPicture *vp;

    /* wait until we have a new pic */
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(this->pictq_mutex);
        while(this->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !this->mQuit)
            this->pictq_cond.wait(&this->pictq_mutex, 1);
    }
    if(this->mQuit)
        return -1;

    this->pictq_mutex.lock();

    // windex is set to 0 initially
    vp = &this->pictq[this->pictq_windex];

    // Convert the image into RGBA format
    // TODO: we could do this in a pixel shader instead, if the source format
    // matches a commonly used format (ie YUV420P)
    if(this->sws_context == NULL)
    {
        int w = (*this->video_st)->codec->width;
        int h = (*this->video_st)->codec->height;
        this->sws_context = sws_getContext(w, h, (*this->video_st)->codec->pix_fmt,
                                           w, h, AV_PIX_FMT_RGBA, SWS_BICUBIC,
                                           NULL, NULL, NULL);
        if(this->sws_context == NULL)
            throw std::runtime_error("Cannot initialize the conversion context!\n");
    }

    vp->pts = pts;
    vp->data.resize((*this->video_st)->codec->width * (*this->video_st)->codec->height * 4);

    uint8_t *dst[4] = { &vp->data[0], nullptr, nullptr, nullptr };
    sws_scale(this->sws_context, pFrame->data, pFrame->linesize,
              0, (*this->video_st)->codec->height, dst, this->rgbaFrame->linesize);

    // now we inform our display thread that we have a pic ready
    this->pictq_windex = (this->pictq_windex+1) % VIDEO_PICTURE_ARRAY_SIZE;
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
    frame_delay = av_q2d((*this->video_st)->codec->time_base);

    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    this->video_clock += frame_delay;

    return pts;
}

static void our_free_buffer(void *opaque, uint8_t *data);
/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
static int64_t global_video_pkt_pts = AV_NOPTS_VALUE;
static int our_get_buffer(struct AVCodecContext *c, AVFrame *pic, int flags)
{
    AVBufferRef *ref;
    int ret = avcodec_default_get_buffer2(c, pic, flags);
    int64_t *pts = (int64_t*)av_malloc(sizeof(int64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    ref = av_buffer_create((uint8_t *)pic->opaque, sizeof(int64_t), our_free_buffer, pic->buf[0], flags);
    pic->buf[0] = ref;
    return ret;
}
static void our_free_buffer(void *opaque, uint8_t *data)
{
    AVBufferRef *ref = (AVBufferRef *)opaque;
    av_buffer_unref(&ref);
    av_free(data);
}

class VideoThread : public OpenThreads::Thread
{
public:
    VideoThread(VideoState* self)
        : mVideoState(self)
    {
        start();
    }

    virtual void run()
    {
        VideoState* self = mVideoState;
        AVPacket pkt1, *packet = &pkt1;
        int frameFinished;
        AVFrame *pFrame;

        pFrame = av_frame_alloc();

        self->rgbaFrame = av_frame_alloc();
        avpicture_alloc((AVPicture*)self->rgbaFrame, AV_PIX_FMT_RGBA, (*self->video_st)->codec->width, (*self->video_st)->codec->height);

        while(self->videoq.get(packet, self) >= 0)
        {
            if(packet->data == flush_pkt.data)
            {
                avcodec_flush_buffers((*self->video_st)->codec);

                self->pictq_mutex.lock();
                self->pictq_size = 0;
                self->pictq_rindex = 0;
                self->pictq_windex = 0;
                self->pictq_mutex.unlock();

                self->frame_last_pts = packet->pts * av_q2d((*self->video_st)->time_base);
                global_video_pkt_pts = static_cast<int64_t>(self->frame_last_pts);
                continue;
            }

            // Save global pts to be stored in pFrame
            global_video_pkt_pts = packet->pts;
            // Decode video frame
            if(avcodec_decode_video2((*self->video_st)->codec, pFrame, &frameFinished, packet) < 0)
                throw std::runtime_error("Error decoding video frame");

            double pts = 0;
            if(packet->dts != AV_NOPTS_VALUE)
                pts = static_cast<double>(packet->dts);
            else if(pFrame->opaque && *(int64_t*)pFrame->opaque != AV_NOPTS_VALUE)
                pts = static_cast<double>(*(int64_t*)pFrame->opaque);
            pts *= av_q2d((*self->video_st)->time_base);

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

private:
    VideoState* mVideoState;
};

class ParseThread : public OpenThreads::Thread
{
public:
    ParseThread(VideoState* self)
        : mVideoState(self)
    {
        start();
    }

    virtual void run()
    {
        VideoState* self = mVideoState;

        AVFormatContext *pFormatCtx = self->format_ctx;
        AVPacket pkt1, *packet = &pkt1;

        try
        {
            if(!self->video_st && !self->audio_st)
                throw std::runtime_error("No streams to decode");

            // main decode loop
            while(!self->mQuit)
            {
                if(self->mSeekRequested)
                {
                    uint64_t seek_target = self->mSeekPos;
                    int streamIndex = -1;

                    int videoStreamIndex = -1;;
                    int audioStreamIndex = -1;
                    if (self->video_st)
                        videoStreamIndex = self->video_st - self->format_ctx->streams;
                    if (self->audio_st)
                        audioStreamIndex = self->audio_st - self->format_ctx->streams;

                    if(videoStreamIndex >= 0)
                        streamIndex = videoStreamIndex;
                    else if(audioStreamIndex >= 0)
                        streamIndex = audioStreamIndex;

                    uint64_t timestamp = seek_target;

                    // QtCreator's highlighter doesn't like AV_TIME_BASE_Q's {} initializer for some reason
                    AVRational avTimeBaseQ = AVRational(); // = AV_TIME_BASE_Q;
                    avTimeBaseQ.num = 1;
                    avTimeBaseQ.den = AV_TIME_BASE;

                    if(streamIndex >= 0)
                        timestamp = av_rescale_q(seek_target, avTimeBaseQ, self->format_ctx->streams[streamIndex]->time_base);

                    // AVSEEK_FLAG_BACKWARD appears to be needed, otherwise ffmpeg may seek to a keyframe *after* the given time
                    // we want to seek to any keyframe *before* the given time, so we can continue decoding as normal from there on
                    if(av_seek_frame(self->format_ctx, streamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0)
                        std::cerr << "Error seeking " << self->format_ctx->filename << std::endl;
                    else
                    {
                        // Clear the packet queues and put a special packet with the new clock time
                        if(audioStreamIndex >= 0)
                        {
                            self->audioq.clear();
                            flush_pkt.pts = av_rescale_q(seek_target, avTimeBaseQ,
                                self->format_ctx->streams[audioStreamIndex]->time_base);
                            self->audioq.put(&flush_pkt);
                        }
                        if(videoStreamIndex >= 0)
                        {
                            self->videoq.clear();
                            flush_pkt.pts = av_rescale_q(seek_target, avTimeBaseQ,
                                self->format_ctx->streams[videoStreamIndex]->time_base);
                            self->videoq.put(&flush_pkt);
                        }
                        self->pictq_mutex.lock();
                        self->pictq_size = 0;
                        self->pictq_rindex = 0;
                        self->pictq_windex = 0;
                        self->pictq_mutex.unlock();
                        self->mExternalClock.set(seek_target);
                    }
                    self->mSeekRequested = false;
                }


                if((self->audio_st && self->audioq.size > MAX_AUDIOQ_SIZE) ||
                   (self->video_st && self->videoq.size > MAX_VIDEOQ_SIZE))
                {
                    OpenThreads::Thread::microSleep(10 * 1000);
                    continue;
                }

                if(av_read_frame(pFormatCtx, packet) < 0)
                {
                    if (self->audioq.nb_packets == 0 && self->videoq.nb_packets == 0 && self->pictq_size == 0)
                        self->mVideoEnded = true;
                    continue;
                }
                else
                    self->mVideoEnded = false;

                // Is this a packet from the video stream?
                if(self->video_st && packet->stream_index == self->video_st-pFormatCtx->streams)
                    self->videoq.put(packet);
                else if(self->audio_st && packet->stream_index == self->audio_st-pFormatCtx->streams)
                    self->audioq.put(packet);
                else
                    av_free_packet(packet);
            }
        }
        catch(std::exception& e) {
            std::cerr << "An error occurred playing the video: " << e.what () << std::endl;
        }

        self->mQuit = true;
    }

private:
    VideoState* mVideoState;
};


bool VideoState::update()
{
    this->video_refresh();
    return !this->mVideoEnded;
}


int VideoState::stream_open(int stream_index, AVFormatContext *pFormatCtx)
{
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
        this->audio_st = pFormatCtx->streams + stream_index;

        if (!mAudioFactory)
        {
            std::cerr << "No audio factory registered, can not play audio stream" << std::endl;
            avcodec_close((*this->audio_st)->codec);
            this->audio_st = NULL;
            return -1;
        }

        mAudioDecoder = mAudioFactory->createDecoder(this);
        if (!mAudioDecoder.get())
        {
            std::cerr << "Failed to create audio decoder, can not play audio stream" << std::endl;
            avcodec_close((*this->audio_st)->codec);
            this->audio_st = NULL;
            return -1;
        }
        mAudioDecoder->setupFormat();
        break;

    case AVMEDIA_TYPE_VIDEO:
        this->video_st = pFormatCtx->streams + stream_index;

        codecCtx->get_buffer2 = our_get_buffer;
        this->video_thread.reset(new VideoThread(this));
        break;

    default:
        break;
    }

    return 0;
}

void VideoState::init(std::shared_ptr<std::istream> inputstream, const std::string &name)
{
    int video_index = -1;
    int audio_index = -1;
    unsigned int i;

    this->av_sync_type = AV_SYNC_DEFAULT;
    this->mQuit = false;

    this->stream = inputstream;
    if(!this->stream.get())
        throw std::runtime_error("Failed to open video resource");

    AVIOContext *ioCtx = avio_alloc_context(NULL, 0, 0, this, istream_read, istream_write, istream_seek);
    if(!ioCtx) throw std::runtime_error("Failed to allocate AVIOContext");

    this->format_ctx = avformat_alloc_context();
    if(this->format_ctx)
        this->format_ctx->pb = ioCtx;

    // Open video file
    ///
    /// format_ctx->pb->buffer must be freed by hand,
    /// if not, valgrind will show memleak, see:
    ///
    /// https://trac.ffmpeg.org/ticket/1357
    ///
    if(!this->format_ctx || avformat_open_input(&this->format_ctx, name.c_str(), NULL, NULL))
    {
        if (this->format_ctx != NULL)
        {
          if (this->format_ctx->pb != NULL)
          {
              av_free(this->format_ctx->pb->buffer);
              this->format_ctx->pb->buffer = NULL;

              av_free(this->format_ctx->pb);
              this->format_ctx->pb = NULL;
          }
        }
        // "Note that a user-supplied AVFormatContext will be freed on failure."
        this->format_ctx = NULL;
        av_free(ioCtx);
        throw std::runtime_error("Failed to open video input");
    }

    // Retrieve stream information
    if(avformat_find_stream_info(this->format_ctx, NULL) < 0)
        throw std::runtime_error("Failed to retrieve stream information");

    // Dump information about file onto standard error
    av_dump_format(this->format_ctx, 0, name.c_str(), 0);

    for(i = 0;i < this->format_ctx->nb_streams;i++)
    {
        if(this->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0)
            video_index = i;
        if(this->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0)
            audio_index = i;
    }

    mExternalClock.set(0);

    if(audio_index >= 0)
        this->stream_open(audio_index, this->format_ctx);

    if(video_index >= 0)
    {
        this->stream_open(video_index, this->format_ctx);
    }


    this->parse_thread.reset(new ParseThread(this));
}

void VideoState::deinit()
{
    this->mQuit = true;

    this->audioq.flush();
    this->videoq.flush();

    mAudioDecoder.reset();

    if (this->parse_thread.get())
    {
        this->parse_thread->join();
        this->parse_thread.reset();
    }
    if (this->video_thread.get())
    {
        this->video_thread->join();
        this->video_thread.reset();
    }

    if(this->audio_st)
        avcodec_close((*this->audio_st)->codec);
    this->audio_st = NULL;
    if(this->video_st)
        avcodec_close((*this->video_st)->codec);
    this->video_st = NULL;

    if(this->sws_context)
        sws_freeContext(this->sws_context);
    this->sws_context = NULL;

    if(this->format_ctx)
    {
        ///
        /// format_ctx->pb->buffer must be freed by hand,
        /// if not, valgrind will show memleak, see:
        ///
        /// https://trac.ffmpeg.org/ticket/1357
        ///
        if (this->format_ctx->pb != NULL)
        {
            av_free(this->format_ctx->pb->buffer);
            this->format_ctx->pb->buffer = NULL;

            av_free(this->format_ctx->pb);
            this->format_ctx->pb = NULL;
        }
        avformat_close_input(&this->format_ctx);
    }

    if (mTexture)
    {
        // reset Image separately, it's pointing to *this and there might still be outside references to mTexture
        mTexture->setImage(NULL);
        mTexture = NULL;
    }
}

double VideoState::get_external_clock()
{
    return mExternalClock.get() / 1000000.0;
}

double VideoState::get_master_clock()
{
    if(this->av_sync_type == AV_SYNC_VIDEO_MASTER)
        return this->get_video_clock();
    if(this->av_sync_type == AV_SYNC_AUDIO_MASTER)
        return this->get_audio_clock();
    return this->get_external_clock();
}

double VideoState::get_video_clock()
{
    return this->frame_last_pts;
}

double VideoState::get_audio_clock()
{
    if (!mAudioDecoder.get())
        return 0.0;
    return mAudioDecoder->getAudioClock();
}

void VideoState::setPaused(bool isPaused)
{
    this->mPaused = isPaused;
    mExternalClock.setPaused(isPaused);
}

void VideoState::seekTo(double time)
{
    time = std::max(0.0, time);
    time = std::min(getDuration(), time);
    mSeekPos = (uint64_t) (time * AV_TIME_BASE);
    mSeekRequested = true;
}

double VideoState::getDuration()
{
    return this->format_ctx->duration / 1000000.0;
}


ExternalClock::ExternalClock()
    : mTimeBase(av_gettime())
    , mPausedAt(0)
    , mPaused(false)
{
}

void ExternalClock::setPaused(bool paused)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    if (mPaused == paused)
        return;
    if (paused)
    {
        mPausedAt = av_gettime() - mTimeBase;
    }
    else
        mTimeBase = av_gettime() - mPausedAt;
    mPaused = paused;
}

uint64_t ExternalClock::get()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    if (mPaused)
        return mPausedAt;
    else
        return av_gettime() - mTimeBase;
}

void ExternalClock::set(uint64_t time)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
    mTimeBase = av_gettime() - time;
    mPausedAt = time;
}

}

