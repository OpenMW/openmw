#ifndef VIDEOPLAYER_VIDEOSTATE_H
#define VIDEOPLAYER_VIDEOSTATE_H

#include <cstdint>
#include <atomic>
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>

#include <osg/ref_ptr>
namespace osg
{
    class Texture2D;
}

#if defined(_MSC_VER)
    #pragma warning (push)
    #pragma warning (disable : 4244)
#endif

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

#if defined(_MSC_VER)
    #pragma warning (pop)
#endif

#include "videodefs.hpp"

#define VIDEO_PICTURE_QUEUE_SIZE 50

#define FFMPEG_5_OR_GREATER (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100))
#define FFMPEG_CONST_WRITEPACKET (LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 12, 100))

extern "C"
{
    struct SwsContext;
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

    std::mutex mMutex;

    void setPaused(bool paused);
    uint64_t get();
    void set(uint64_t time);
};

class PacketList
{
public:
    AVPacket* pkt = nullptr;
    PacketList *next = nullptr;
};

struct PacketQueue {
    PacketQueue()
      : first_pkt(nullptr), last_pkt(nullptr), flushing(false), nb_packets(0), size(0)
    { }
    ~PacketQueue()
    { clear(); }

    PacketList *first_pkt, *last_pkt;
    std::atomic<bool> flushing;
    std::atomic<int> nb_packets;
    std::atomic<int> size;

    std::mutex mutex;
    std::condition_variable cond;

    void put(AVPacket *pkt);
    int get(AVPacket *pkt, VideoState *is);

    void flush();
    void clear();
};

struct VideoPicture {
    VideoPicture() : pts(0.0)
    { }

    struct AVFrameDeleter {
        void operator()(AVFrame* frame) const;
    };

    // Sets frame dimensions.
    // Must be called before writing to `rgbaFrame`.
    // Return -1 on error.
    int set_dimensions(int w, int h);

    std::unique_ptr<AVFrame, AVFrameDeleter> rgbaFrame;
    double pts;
};

struct VideoState {
    VideoState();
    ~VideoState();

    void setAudioFactory(MovieAudioFactory* factory);

    void init(std::unique_ptr<std::istream>&& inputstream, const std::string& name);
    void deinit();

    void setPaused(bool isPaused);
    void seekTo(double time);

    double getDuration() const;

    int stream_open(int stream_index, AVFormatContext *pFormatCtx);

    bool update();

    static void video_thread_loop(VideoState *is);
    static void decode_thread_loop(VideoState *is);

    void video_display(VideoPicture* vp);
    void video_refresh();

    int queue_picture(const AVFrame &pFrame, double pts);
    double synchronize_video(const AVFrame &src_frame, double pts);

    double get_audio_clock();
    double get_video_clock() const;
    double get_external_clock();
    double get_master_clock();

    static int istream_read(void *user_data, uint8_t *buf, int buf_size);

#if FFMPEG_CONST_WRITEPACKET
    static int istream_write(void *, const unsigned char *, int);
#else
    static int istream_write(void *, uint8_t *, int);
#endif

    static int64_t istream_seek(void *user_data, int64_t offset, int whence);

    osg::ref_ptr<osg::Texture2D> mTexture;

    MovieAudioFactory* mAudioFactory;
    std::unique_ptr<MovieAudioDecoder> mAudioDecoder;

    ExternalClock mExternalClock;

    std::unique_ptr<std::istream> stream;
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
    int sws_context_w, sws_context_h;
    std::array<VideoPicture, VIDEO_PICTURE_QUEUE_SIZE+1> pictq;  // allocate one extra to make sure we do not overwrite the osg::Image currently set on the texture
    int pictq_size;
    unsigned long pictq_rindex, pictq_windex;
    std::mutex pictq_mutex;
    std::condition_variable pictq_cond;

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
