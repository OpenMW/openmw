#include "videoplayer.hpp"


#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwsound/sound_decoder.hpp"
#include "../mwsound/sound.hpp"


namespace MWRender
{

    int OgreResource_Read(void *user_data, uint8_t *buf, int buf_size)
    {
        Ogre::DataStreamPtr stream = static_cast<VideoState*>(user_data)->stream;
        return stream->read(buf, buf_size);
    }

    int OgreResource_Write(void *user_data, uint8_t *buf, int buf_size)
    {
        Ogre::DataStreamPtr stream = static_cast<VideoState*>(user_data)->stream;
        return stream->write(buf, buf_size);
    }

    int64_t OgreResource_Seek(void *user_data, int64_t offset, int whence)
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

    void packet_queue_init(PacketQueue *q)
    { memset(q, 0, sizeof(PacketQueue)); }

    int packet_queue_put(PacketQueue *q, AVPacket *pkt)
    {
        AVPacketList *pkt1;
        if(av_dup_packet(pkt) < 0)
            return -1;

        pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
        if(!pkt1) return -1;
        pkt1->pkt = *pkt;
        pkt1->next = NULL;

        q->mutex.lock ();

        if (!q->last_pkt)
            q->first_pkt = pkt1;
        else
            q->last_pkt->next = pkt1;
        q->last_pkt = pkt1;
        q->nb_packets++;
        q->size += pkt1->pkt.size;
        q->cond.notify_one();
        q->mutex.unlock ();
        return 0;
    }

    static int packet_queue_get(PacketQueue *q, AVPacket *pkt, VideoState *is, int block)
    {
        AVPacketList *pkt1;
        int ret;

        boost::unique_lock<boost::mutex> lock(q->mutex);

        for(;;)
        {
            if(is->quit)
            {
                ret = -1;
                break;
            }

            pkt1 = q->first_pkt;
            if (pkt1)
            {
                q->first_pkt = pkt1->next;
                if (!q->first_pkt)
                    q->last_pkt = NULL;
                q->nb_packets--;
                q->size -= pkt1->pkt.size;
                *pkt = pkt1->pkt;
                av_free(pkt1);
                ret = 1;
                break;
            }

            if (!block)
            {
                ret = 0;
                break;
            }

            q->cond.wait(lock);
        }

        return ret;
    }

    void PacketQueue::flush()
    {
        AVPacketList *pkt, *pkt1;

        this->mutex.lock();
        for(pkt = this->first_pkt; pkt != NULL; pkt = pkt1) {
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


    double get_audio_clock(VideoState *is)
    {
        return is->AudioTrack->getTimeOffset();
    }

    double get_video_clock(VideoState *is)
    {
        double delta;

        delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
        return is->video_current_pts + delta;
    }

    double get_external_clock(VideoState *is)
    {
        return av_gettime() / 1000000.0;
    }

    double get_master_clock(VideoState *is)
    {
        if(is->av_sync_type == AV_SYNC_VIDEO_MASTER)
            return get_video_clock(is);
        if(is->av_sync_type == AV_SYNC_AUDIO_MASTER)
            return get_audio_clock(is);
        return get_external_clock(is);
    }

class MovieAudioDecoder : public MWSound::Sound_Decoder
{
    static void fail(const std::string &str)
    {
        throw std::runtime_error(str);
    }

    VideoState *is;

    /* Add or subtract samples to get a better sync, return new
     * audio buffer size */
    int synchronize_audio(uint8_t *samples, int samples_size, double pts)
    {
        if(is->av_sync_type == AV_SYNC_AUDIO_MASTER)
            return samples_size;

        double diff, avg_diff, ref_clock;
        int wanted_size, min_size, max_size, n;
        // int nb_samples;

        n = 2 * is->audio_st->codec->channels;

        ref_clock = get_master_clock(is);
        diff = get_audio_clock(is) - ref_clock;
        if(diff < AV_NOSYNC_THRESHOLD)
        {
            // accumulate the diffs
            is->audio_diff_cum = diff + is->audio_diff_avg_coef *
                                        is->audio_diff_cum;
            if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
                is->audio_diff_avg_count++;
            else
            {
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
                if(fabs(avg_diff) >= is->audio_diff_threshold)
                {
                    wanted_size = samples_size + ((int)(diff * is->audio_st->codec->sample_rate) * n);
                    min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
                    max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);

                    if(wanted_size < min_size)
                        wanted_size = min_size;
                    else if (wanted_size > max_size)
                        wanted_size = max_size;

                    if(wanted_size < samples_size)
                    {
                        /* remove samples */
                        samples_size = wanted_size;
                    }
                    else if(wanted_size > samples_size)
                    {
                        uint8_t *samples_end, *q;
                        int nb;
                        /* add samples by copying final sample*/
                        nb = (samples_size - wanted_size);
                        samples_end = samples + samples_size - n;
                        q = samples_end + n;
                        while(nb > 0)
                        {
                            memcpy(q, samples_end, n);
                            q += n;
                            nb -= n;
                        }
                        samples_size = wanted_size;
                    }
                }
            }
        }
        else
        {
            /* difference is TOO big; reset diff stuff */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
        }

        return samples_size;
    }

    int audio_decode_frame(uint8_t *audio_buf, int buf_size, double *pts_ptr)
    {
        AVPacket *pkt = &is->audio_pkt;
        int len1, data_size, n;
        double pts;

        for(;;)
        {
            while(is->audio_pkt_size > 0)
            {
                data_size = buf_size;

                len1 = avcodec_decode_audio3(is->audio_st->codec,
                                             (int16_t*)audio_buf, &data_size, pkt);
                if(len1 < 0)
                {
                    /* if error, skip frame */
                    is->audio_pkt_size = 0;
                    break;
                }
                is->audio_pkt_data += len1;
                is->audio_pkt_size -= len1;
                if(data_size <= 0)
                {
                    /* No data yet, get more frames */
                    continue;
                }
                pts = is->audio_clock;
                *pts_ptr = pts;
                n = 2 * is->audio_st->codec->channels;
                is->audio_clock += (double)data_size /
                                   (double)(n * is->audio_st->codec->sample_rate);

                /* We have data, return it and come back for more later */
                return data_size;
            }
            if(pkt->data)
                av_free_packet(pkt);

            if(is->quit)
                return -1;

            /* next packet */
            if(packet_queue_get(&is->audioq, pkt, is, 1) < 0)
                return -1;

            is->audio_pkt_data = pkt->data;
            is->audio_pkt_size = pkt->size;
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
    MovieAudioDecoder(VideoState *_is) : is(_is) { }

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
        size_t total = 0;

        while(total < len)
        {
            if(is->audio_buf_index >= is->audio_buf_size)
            {
                int audio_size;
                double pts;
                /* We have already sent all our data; get more */
                audio_size = audio_decode_frame(is->audio_buf, sizeof(is->audio_buf), &pts);
                if(audio_size < 0)
                {
                    /* If error, we're done */
                    break;
                }

                audio_size = synchronize_audio(is->audio_buf, audio_size, pts);
                is->audio_buf_size = audio_size;
                is->audio_buf_index = 0;
            }

            size_t len1 = std::min<size_t>(is->audio_buf_size - is->audio_buf_index,
                                           len - total);
            memcpy(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1);

            total += len1;
            stream += len1;
            is->audio_buf_index += len1;
        }

        return total;
    }

    size_t getSampleOffset()
    {
        return (size_t)(is->audio_clock*is->audio_st->codec->sample_rate);
    }
};


    void timer_callback (boost::system_time t, VideoState* is)
    {
        boost::this_thread::sleep(t);
        is->refresh++;
    }

    /* schedule a video refresh in 'delay' ms */
    static void schedule_refresh(VideoState *is, int delay)
    {
        boost::system_time t = boost::get_system_time() + boost::posix_time::milliseconds(delay);
        boost::thread(boost::bind(&timer_callback, t, is)).detach();
    }

    void video_display(VideoState *is)
    {
        VideoPicture *vp;

        vp = &is->pictq[is->pictq_rindex];

        if (is->video_st->codec->width != 0 && is->video_st->codec->height != 0)
        {
            Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton ().getByName("VideoTexture");
            if (texture.isNull () || static_cast<int>(texture->getWidth()) != is->video_st->codec->width
                    || static_cast<int>(texture->getHeight()) != is->video_st->codec->height)
            {
                Ogre::TextureManager::getSingleton ().remove ("VideoTexture");
                texture = Ogre::TextureManager::getSingleton().createManual(
                                        "VideoTexture",
                                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                        Ogre::TEX_TYPE_2D,
                                        is->video_st->codec->width, is->video_st->codec->height,
                                        0,
                                        Ogre::PF_BYTE_RGBA,
                                        Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
            }
            Ogre::PixelBox pb(is->video_st->codec->width, is->video_st->codec->height, 1, Ogre::PF_BYTE_RGBA, vp->data);
            Ogre::HardwarePixelBufferSharedPtr buffer = texture->getBuffer();
            buffer->blitFromMemory(pb);
            is->display_ready = 1;
        }

        free(vp->data);
    }


    void video_refresh_timer(void *userdata)
    {
        VideoState *is = (VideoState *)userdata;
        VideoPicture *vp;
        double actual_delay, delay, sync_threshold, ref_clock, diff;

        if(is->video_st)
        {
            if(is->pictq_size == 0)
                schedule_refresh(is, 1);
            else
            {
                vp = &is->pictq[is->pictq_rindex];

                is->video_current_pts = vp->pts;
                is->video_current_pts_time = av_gettime();

                delay = vp->pts - is->frame_last_pts; /* the pts from last time */
                if(delay <= 0 || delay >= 1.0) {
                    /* if incorrect delay, use previous one */
                    delay = is->frame_last_delay;
                }
                /* save for next time */
                is->frame_last_delay = delay;
                is->frame_last_pts = vp->pts;

                /* update delay to sync to audio if not master source */
                if(is->av_sync_type != AV_SYNC_VIDEO_MASTER)
                {
                    ref_clock = get_master_clock(is);
                    diff = vp->pts - ref_clock;

                    /* Skip or repeat the frame. Take delay into account
                       FFPlay still doesn't "know if this is the best guess." */
                    sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
                    if(fabs(diff) < AV_NOSYNC_THRESHOLD)
                    {
                        if(diff <= -sync_threshold)
                            delay = 0;
                        else if(diff >= sync_threshold)
                            delay = 2 * delay;
                    }
                }

                is->frame_timer += delay;
                /* computer the REAL delay */
                actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
                if(actual_delay < 0.010)
                {
                    /* Really it should skip the picture instead */
                    actual_delay = 0.010;
                }
                schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));

                /* show the picture! */
                video_display(is);

                /* update queue for next picture! */
                if(++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE)
                    is->pictq_rindex = 0;
                is->pictq_mutex.lock();
                is->pictq_size--;
                is->pictq_cond.notify_one ();
                is->pictq_mutex.unlock ();
            }
        }
        else
            schedule_refresh(is, 100);
    }

    int queue_picture(VideoState *is, AVFrame *pFrame, double pts)
    {
        VideoPicture *vp;

        /* wait until we have a new pic */
        {
            boost::unique_lock<boost::mutex> lock(is->pictq_mutex);
            while(is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !is->quit)
                is->pictq_cond.timed_wait(lock, boost::posix_time::milliseconds(1));
        }

        if(is->quit)
            return -1;

        // windex is set to 0 initially
        vp = &is->pictq[is->pictq_windex];

        // Convert the image into YUV format that SDL uses
        if(is->sws_context == NULL)
        {
            int w = is->video_st->codec->width;
            int h = is->video_st->codec->height;
            is->sws_context = sws_getContext(w, h, is->video_st->codec->pix_fmt,
                                             w, h, PIX_FMT_RGBA, SWS_BICUBIC,
                                             NULL, NULL, NULL);
            if(is->sws_context == NULL)
                throw std::runtime_error("Cannot initialize the conversion context!\n");
        }

        vp->data =(uint8_t*) malloc(is->video_st->codec->width * is->video_st->codec->height * 4);

        sws_scale(is->sws_context, pFrame->data, pFrame->linesize,
                  0, is->video_st->codec->height, &vp->data, is->rgbaFrame->linesize);

        vp->pts = pts;

        // now we inform our display thread that we have a pic ready
        if(++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
            is->pictq_windex = 0;
        is->pictq_mutex.lock();
        is->pictq_size++;
        is->pictq_mutex.unlock();

        return 0;
    }

    double synchronize_video(VideoState *is, AVFrame *src_frame, double pts)
    {
        double frame_delay;

        if(pts != 0)
        {
            /* if we have pts, set video clock to it */
            is->video_clock = pts;
        }
        else
        {
            /* if we aren't given a pts, set it to the clock */
            pts = is->video_clock;
        }
        /* update the video clock */
        frame_delay = av_q2d(is->video_st->codec->time_base);
        /* if we are repeating a frame, adjust clock accordingly */
        frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
        is->video_clock += frame_delay;
        return pts;
    }

    uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

    /* These are called whenever we allocate a frame
     * buffer. We use this to store the global_pts in
     * a frame at the time it is allocated.
     */
    int our_get_buffer(struct AVCodecContext *c, AVFrame *pic)
    {
        int ret = avcodec_default_get_buffer(c, pic);
        uint64_t *pts = (uint64_t*)av_malloc(sizeof(uint64_t));
        *pts = global_video_pkt_pts;
        pic->opaque = pts;
        return ret;
    }
    void our_release_buffer(struct AVCodecContext *c, AVFrame *pic)
    {
        if(pic) av_freep(&pic->opaque);
        avcodec_default_release_buffer(c, pic);
    }

    int video_thread(void *arg)
    {
        VideoState *is = (VideoState *)arg;
        AVPacket pkt1, *packet = &pkt1;
        int frameFinished;
        AVFrame *pFrame;
        double pts;

        pFrame = avcodec_alloc_frame();

        is->rgbaFrame = avcodec_alloc_frame();
        avpicture_alloc((AVPicture*)is->rgbaFrame, PIX_FMT_RGBA, is->video_st->codec->width, is->video_st->codec->height);

        for(;;)
        {
            if(packet_queue_get(&is->videoq, packet, is, 1) < 0)
            {
                // means we quit getting packets
                break;
            }
            pts = 0;

            // Save global pts to be stored in pFrame
            global_video_pkt_pts = packet->pts;
            // Decode video frame
            if (avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished, packet) < 0)
                throw std::runtime_error("Error decoding video frame");

            if((uint64_t)pFrame->pts != AV_NOPTS_VALUE)
                pts = pFrame->pts;
            else if((uint64_t)packet->pts != AV_NOPTS_VALUE)
                pts = packet->pts;
            else if(pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)
                pts = *(uint64_t *)pFrame->opaque;
            else
                pts = 0;
            pts *= av_q2d(is->video_st->time_base);

            // Did we get a video frame?
            if(frameFinished)
            {
                pts = synchronize_video(is, pFrame, pts);
                if(queue_picture(is, pFrame, pts) < 0)
                    break;
            }
            av_free_packet(packet);
        }

        av_free(pFrame);

        avpicture_free((AVPicture *)is->rgbaFrame);
        av_free(is->rgbaFrame);

        return 0;
    }

    int stream_component_open(VideoState *is, int stream_index, AVFormatContext *pFormatCtx)
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
            is->audioStream = stream_index;
            is->audio_st = pFormatCtx->streams[stream_index];
            is->audio_buf_size = 0;
            is->audio_buf_index = 0;

            /* averaging filter for audio sync */
            is->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
            is->audio_diff_avg_count = 0;
            /* Correct audio only if larger error than this */
            is->audio_diff_threshold = 2.0 * 0.1/* 100 ms */;

            memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
            packet_queue_init(&is->audioq);

            decoder.reset(new MovieAudioDecoder(is));
            is->AudioTrack = MWBase::Environment::get().getSoundManager()->playTrack(decoder);
            if(!is->AudioTrack)
            {
                is->audioStream = -1;
                avcodec_close(is->audio_st->codec);
                is->audio_st = NULL;
                return -1;
            }
            break;

        case AVMEDIA_TYPE_VIDEO:
            is->videoStream = stream_index;
            is->video_st = pFormatCtx->streams[stream_index];

            is->frame_timer = (double)av_gettime() / 1000000.0;
            is->frame_last_delay = 40e-3;
            is->video_current_pts_time = av_gettime();

            packet_queue_init(&is->videoq);

            codecCtx->get_buffer = our_get_buffer;
            codecCtx->release_buffer = our_release_buffer;
            is->video_thread = boost::thread(video_thread, is);
            break;

        default:
            break;
        }

        return 0;
    }

    int decode_thread(void *arg)
    {
        VideoState *is = (VideoState *)arg;
        try
        {
            AVFormatContext *pFormatCtx = is->format_ctx;
            AVPacket pkt1, *packet = &pkt1;

            if(is->videoStream >= 0 /*|| is->audioStream < 0*/)
            {
                // main decode loop
                for(;;)
                {
                    if(is->quit)
                        break;

                    if((is->audioStream >= 0 && is->audioq.size > MAX_AUDIOQ_SIZE) ||
                        is->videoq.size > MAX_VIDEOQ_SIZE)
                    {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
                        continue;
                    }

                    if(av_read_frame(pFormatCtx, packet) < 0)
                        break;

                    // Is this a packet from the video stream?
                    if(packet->stream_index == is->videoStream)
                        packet_queue_put(&is->videoq, packet);
                    else if(packet->stream_index == is->audioStream)
                        packet_queue_put(&is->audioq, packet);
                    else
                        av_free_packet(packet);
                }
                /* all done - wait for it */
                while(!is->quit)
                {
                    // EOF reached, all packets processed, we can exit now
                    if(is->audioq.nb_packets == 0 && is->videoq.nb_packets == 0)
                        break;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                }
            }

            is->quit = 1;
        }
        catch (std::runtime_error& e)
        {
            std::cerr << "An error occured playing the video: " << e.what () << std::endl;
            is->quit = 1;
        }
        catch (Ogre::Exception& e)
        {
            std::cerr << "An error occured playing the video: " << e.getFullDescription () << std::endl;
            is->quit = 1;
        }

        return 0;
    }

    void init_state(VideoState *is, const std::string& resourceName)
    {
        int video_index = -1;
        int audio_index = -1;
        unsigned int i;

        is->av_sync_type = DEFAULT_AV_SYNC_TYPE;
        is->format_ctx = avformat_alloc_context();
        is->videoStream = -1;
        is->audioStream = -1;
        is->refresh = 0;
        is->quit = 0;

        is->stream = Ogre::ResourceGroupManager::getSingleton ().openResource(resourceName);
        if(is->stream.isNull())
            throw std::runtime_error("Failed to open video resource");

        is->format_ctx->pb = avio_alloc_context(NULL, 0, 0, is, OgreResource_Read, OgreResource_Write, OgreResource_Seek);
        if(!is->format_ctx->pb)
            throw std::runtime_error("Failed to allocate ioContext ");

        // Open video file
        /// \todo leak here, ffmpeg or valgrind bug ?
        if (avformat_open_input(&is->format_ctx, resourceName.c_str(), NULL, NULL))
            throw std::runtime_error("Failed to open video input");

        // Retrieve stream information
        if(avformat_find_stream_info(is->format_ctx, NULL) < 0)
            throw std::runtime_error("Failed to retrieve stream information");

        // Dump information about file onto standard error
        av_dump_format(is->format_ctx, 0, resourceName.c_str(), 0);

        for(i = 0;i < is->format_ctx->nb_streams;i++)
        {
            if(is->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0)
                video_index = i;
            if(is->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0)
                audio_index = i;
        }

        if(audio_index >= 0)
            stream_component_open(is, audio_index, is->format_ctx);
        if(video_index >= 0)
            stream_component_open(is, video_index, is->format_ctx);
    }

    void deinit_state(VideoState *is)
    {
        is->audioq.cond.notify_one ();
        is->videoq.cond.notify_one ();

        is->parse_thread.join();
        is->video_thread.join();

        if(is->audioStream >= 0)
            avcodec_close(is->audio_st->codec);
        if(is->videoStream >= 0)
            avcodec_close(is->video_st->codec);

        sws_freeContext(is->sws_context);

        AVIOContext *ioContext = is->format_ctx->pb;
        avformat_close_input(&is->format_ctx);
        av_free(ioContext);
    }

    VideoPlayer::VideoPlayer(Ogre::SceneManager* sceneMgr)
        : mState(NULL)
        , mSceneMgr(sceneMgr)
    {
        mVideoMaterial = Ogre::MaterialManager::getSingleton ().create("VideoMaterial", "General");
        mVideoMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        mVideoMaterial->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
        mVideoMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        mVideoMaterial->getTechnique(0)->getPass(0)->createTextureUnitState();

        mRectangle = new Ogre::Rectangle2D(true);
        mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
        mRectangle->setMaterial("VideoMaterial");
        mRectangle->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY+1);
        // Use infinite AAB to always stay visible
        Ogre::AxisAlignedBox aabInf;
        aabInf.setInfinite();
        mRectangle->setBoundingBox(aabInf);
        // Attach background to the scene
        Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(mRectangle);
        mRectangle->setVisible(false);
        mRectangle->setVisibilityFlags (0x1);
    }

    VideoPlayer::~VideoPlayer ()
    {
        if (mState)
            close();
    }

    void VideoPlayer::playVideo (const std::string &resourceName)
    {
        // Register all formats and codecs
        av_register_all();

        if (mState)
            close();

        mRectangle->setVisible(true);

        MWBase::Environment::get().getWindowManager ()->pushGuiMode (MWGui::GM_Video);

        // Turn off rendering except the GUI
        mSceneMgr->clearSpecialCaseRenderQueues();
        // SCRQM_INCLUDE with RENDER_QUEUE_OVERLAY does not work.
        for (int i = 0; i < Ogre::RENDER_QUEUE_MAX; ++i)
        {
            if (i > 0 && i < 96)
                mSceneMgr->addSpecialCaseRenderQueue(i);
        }
        mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

        MWBase::Environment::get().getSoundManager()->pauseAllSounds();

        mState = new VideoState;

        init_state(mState, resourceName);

        schedule_refresh(mState, 40);
        mState->parse_thread = boost::thread(decode_thread, mState);
    }

    void VideoPlayer::update ()
    {
        if(mState)
        {
            if(mState->quit)
                close();
            else if(mState->refresh)
            {
                video_refresh_timer(mState);
                mState->refresh--;
            }
        }

        if (mState && mState->display_ready && !Ogre::TextureManager::getSingleton ().getByName ("VideoTexture").isNull ())
            mVideoMaterial->getTechnique(0)->getPass(0)->getTextureUnitState (0)->setTextureName ("VideoTexture");
        else
            mVideoMaterial->getTechnique(0)->getPass(0)->getTextureUnitState (0)->setTextureName ("black.png");
    }

    void VideoPlayer::close()
    {
        mState->quit = 1;
        deinit_state(mState);

        delete mState;
        mState = NULL;

        MWBase::Environment::get().getSoundManager()->resumeAllSounds();

        mRectangle->setVisible (false);
        MWBase::Environment::get().getWindowManager ()->removeGuiMode (MWGui::GM_Video);

        mSceneMgr->clearSpecialCaseRenderQueues();
        mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
    }

    bool VideoPlayer::isPlaying ()
    {
        return mState != NULL;
    }
}
