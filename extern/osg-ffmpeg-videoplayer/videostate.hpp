#ifndef VIDEOPLAYER_VIDEOSTATE_H
#define VIDEOPLAYER_VIDEOSTATE_H

#include <stdint.h>
#include <atomic>
#include <vector>
#include <memory>
#include <string>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>
#include <OpenThreads/Condition>

#include <osg/ref_ptr>
namespace osg
{
    class Texture2D;
}

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>

// From version 54.56 binkaudio encoding format changed from S16 to FLTP. See:
// https://gitorious.org/ffmpeg/ffmpeg/commit/7bfd1766d1c18f07b0a2dd042418a874d49ea60d
// https://ffmpeg.zeranoe.com/forum/viewtopic.php?f=15&t=872
#include <libswresample/swresample.h>
}

#include "videodefs.hpp"

#define VIDEO_PICTURE_QUEUE_SIZE 50
// allocate one extra to make sure we do not overwrite the osg::Image currently set on the texture
#define VIDEO_PICTURE_ARRAY_SIZE (VIDEO_PICTURE_QUEUE_SIZE+1)

extern "C"
{
    struct SwsContext;
    struct AVPacketList;
    struct AVPacket;
    struct AVFormatContext;
    struct AVStream;
    struct AVFrame;
}

namespace Video
{

struct VideoState;

class MovieAudioFactory;
class MovieAudioDecoder;
class VideoThread;
class ParseThread;

struct ExternalClock
{
    ExternalClock();

    uint64_t mTimeBase;
    uint64_t mPausedAt;
    bool mPaused;

    OpenThreads::Mutex mMutex;

    void setPaused(bool paused);
    uint64_t get();
    void set(uint64_t time);
};

struct PacketQueue {
    PacketQueue()
      : first_pkt(NULL), last_pkt(NULL), flushing(false), nb_packets(0), size(0)
    { }
    ~PacketQueue()
    { clear(); }

    AVPacketList *first_pkt, *last_pkt;
    std::atomic<bool> flushing;
    std::atomic<int> nb_packets;
    std::atomic<int> size;

    OpenThreads::Mutex mutex;
    OpenThreads::Condition cond;

    void put(AVPacket *pkt);
    int get(AVPacket *pkt, VideoState *is);

    void flush();
    void clear();
};

struct VideoPicture {
    VideoPicture() : pts(0.0)
    { }

    std::vector<uint8_t> data;
    double pts;
};

struct VideoState {
    VideoState();
    ~VideoState();

    void setAudioFactory(MovieAudioFactory* factory);

    void init(std::shared_ptr<std::istream> inputstream, const std::string& name);
    void deinit();

    void setPaused(bool isPaused);
    void seekTo(double time);

    double getDuration();

    int stream_open(int stream_index, AVFormatContext *pFormatCtx);

    bool update();

    static void video_thread_loop(VideoState *is);
    static void decode_thread_loop(VideoState *is);

    void video_display(VideoPicture* vp);
    void video_refresh();

    int queue_picture(AVFrame *pFrame, double pts);
    double synchronize_video(AVFrame *src_frame, double pts);

    double get_audio_clock();
    double get_video_clock();
    double get_external_clock();
    double get_master_clock();

    static int istream_read(void *user_data, uint8_t *buf, int buf_size);
    static int istream_write(void *user_data, uint8_t *buf, int buf_size);
    static int64_t istream_seek(void *user_data, int64_t offset, int whence);

    osg::ref_ptr<osg::Texture2D> mTexture;

    MovieAudioFactory* mAudioFactory;
    std::shared_ptr<MovieAudioDecoder> mAudioDecoder;

    ExternalClock mExternalClock;

    std::shared_ptr<std::istream> stream;
    AVFormatContext* format_ctx;
    AVCodecContext* video_ctx;
    AVCodecContext* audio_ctx;

    int av_sync_type;

    AVStream**  audio_st;
    PacketQueue audioq;

    uint8_t* mFlushPktData;

    AVStream**  video_st;
    double      frame_last_pts;
    double      video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    PacketQueue videoq;
    SwsContext*  sws_context;
    VideoPicture pictq[VIDEO_PICTURE_ARRAY_SIZE];
    AVFrame*     rgbaFrame; // used as buffer for the frame converted from its native format to RGBA
    int          pictq_size, pictq_rindex, pictq_windex;
    OpenThreads::Mutex pictq_mutex;
    OpenThreads::Condition pictq_cond;

    std::unique_ptr<ParseThread> parse_thread;
    std::unique_ptr<VideoThread> video_thread;

    std::atomic<bool> mSeekRequested;
    uint64_t mSeekPos;

    std::atomic<bool> mVideoEnded;
    std::atomic<bool> mPaused;
    std::atomic<bool> mQuit;
};

}

#endif
