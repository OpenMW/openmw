#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>

#include <boost/thread.hpp>


#define __STDC_CONSTANT_MACROS
#include <stdint.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include <cstdio>
#include <cmath>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20
#define VIDEO_PICTURE_QUEUE_SIZE 1
#define DEFAULT_AV_SYNC_TYPE AV_SYNC_VIDEO_MASTER



namespace MWRender
{


    struct PacketQueue {
        PacketQueue () :
            first_pkt(NULL), last_pkt(NULL), nb_packets(0), size(0)
        {}
        AVPacketList *first_pkt, *last_pkt;
        int nb_packets;
        int size;

        boost::mutex mutex;
        boost::condition_variable cond;

        void flush ();
    };
    struct VideoPicture {
        VideoPicture () :
            data(NULL), pts(0)
        {}
        uint8_t* data;

        double pts;
    };

    struct VideoState {
        VideoState () :
            videoStream(-1), audioStream(-1), av_sync_type(0), external_clock(0),
            external_clock_time(0), audio_clock(0), audio_st(NULL), audio_buf_size(0),
            audio_pkt_data(NULL), audio_pkt_size(0), audio_hw_buf_size(0), audio_diff_cum(0), audio_diff_avg_coef(0),
            audio_diff_threshold(0), audio_diff_avg_count(0), frame_timer(0), frame_last_pts(0), frame_last_delay(0),
            video_clock(0), video_current_pts(0), video_current_pts_time(0), video_st(NULL), rgbaFrame(NULL), pictq_size(0),
            pictq_rindex(0), pictq_windex(0), quit(false), refresh(0), sws_context(NULL), display_ready(0)
        {}


        ~VideoState()
        {
            audioq.flush ();
            videoq.flush();

            if (pictq_size >= 1)
                free (pictq[0].data);
        }

        int                      videoStream, audioStream;

        int                      av_sync_type;
        double                  external_clock; /* external clock base */
        int64_t              external_clock_time;
        double                  audio_clock;
        AVStream                *audio_st;
        PacketQueue      audioq;
        DECLARE_ALIGNED(16, uint8_t, audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);
        unsigned int        audio_buf_size;
        unsigned int        audio_buf_index;
        AVPacket                audio_pkt;
        uint8_t              *audio_pkt_data;
        int                      audio_pkt_size;
        int                      audio_hw_buf_size;
        double                  audio_diff_cum; /* used for AV difference average computation */
        double                  audio_diff_avg_coef;
        double                  audio_diff_threshold;
        int                      audio_diff_avg_count;
        double                  frame_timer;
        double                  frame_last_pts;
        double                  frame_last_delay;
        double                  video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
        double                  video_current_pts; ///<current displayed pts (different from video_clock if frame fifos are used)
        int64_t              video_current_pts_time;    ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts
        AVStream                *video_st;
        PacketQueue      videoq;

        Ogre::DataStreamPtr stream;

        SwsContext* sws_context;

        VideoPicture        pictq[VIDEO_PICTURE_QUEUE_SIZE];
        AVFrame*                rgbaFrame; // used as buffer for the frame converted from its native format to RGBA
        int                      pictq_size, pictq_rindex, pictq_windex;

        boost::mutex pictq_mutex;
        boost::condition_variable pictq_cond;

        boost::thread parse_thread;
        boost::thread video_thread;

        std::string resourceName;
        int quit;

        int refresh;
        int display_ready;
    };
    enum {
        AV_SYNC_AUDIO_MASTER,
        AV_SYNC_VIDEO_MASTER,
        AV_SYNC_EXTERNAL_MASTER
    };


    class VideoPlayer
    {

    public:
        VideoPlayer(Ogre::SceneManager* sceneMgr);
        ~VideoPlayer();

        void playVideo (const std::string& resourceName);

        void update();

        void close();

        bool isPlaying();


    private:
        VideoState* mState;

        Ogre::SceneManager* mSceneMgr;
        Ogre::Rectangle2D* mRectangle;
        Ogre::MaterialPtr mVideoMaterial;

    };

}

#endif
