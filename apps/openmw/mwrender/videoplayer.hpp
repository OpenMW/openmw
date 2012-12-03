#ifndef MWRENDER_VIDEOPLAYER_H
#define MWRENDER_VIDEOPLAYER_H

//#ifdef OPENMW_USE_FFMPEG



#include <string>

#include <OgreDataStream.h>
#include <OgreTexture.h>
#include <OgreTimer.h>

namespace Ogre
{
    class Rectangle2D;
    class SceneManager;
    class TextureUnitState;
}

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVStream;
struct AVFrame;
struct SwsContext;
struct AVPacket;
struct AVPacketList;

namespace MWRender
{

    /// A simple queue used to queue raw audio and video data.
    class AVPacketQueue
    {
    public:
        AVPacketQueue();
        int put(AVPacket* pkt);
        int get(AVPacket* pkt, int block);

        bool isEmpty() const { return mNumPackets == 0; }
        int getNumPackets() const { return mNumPackets; }
        int getSize() const { return mSize; }

    private:
        AVPacketList*   mFirstPacket;
        AVPacketList*   mLastPacket;
        int             mNumPackets;
        int             mSize;
    };


    class VideoPlayer
    {
    public:
        VideoPlayer(Ogre::SceneManager* sceneMgr);
        ~VideoPlayer();

        void play (const std::string& resourceName);

        void update();

    private:
        Ogre::Rectangle2D* mRectangle;
        Ogre::TextureUnitState* mTextureUnit;

        Ogre::DataStreamPtr mStream;

        Ogre::TexturePtr mVideoTexture;


    private:

        AVFormatContext* mAvContext;

        Ogre::Timer mTimer;

        AVCodec* mVideoCodec;
        AVCodec* mAudioCodec;

        AVStream*           mVideoStream;
        AVStream*           mAudioStream;
        int                 mVideoStreamId;      ///< ID of the first video stream
        int                 mAudioStreamId;      ///< ID of the first audio stream

        AVFrame* mRawFrame;
        AVFrame* mRGBAFrame;
        SwsContext* mSwsContext;

        double mClock;
        double mVideoClock;
        double mAudioClock;

        AVPacketQueue mVideoPacketQueue;
        AVPacketQueue mAudioPacketQueue;


        void close();
        void deleteContext();


        void throwError(int error);




        bool addToBuffer(); ///< try to add the next audio or video packet into the queue.

        void decodeNextVideoFrame(); ///< decode the next video frame in the queue and display it.
    };

}

//#else
/*


// If FFMPEG not available, dummy implentation that does nothing

namespace MWRender
{

    class VideoPlayer
    {
    public:
        VideoPlayer(Ogre::SceneManager* sceneMgr){}

        void play (const std::string& resourceName) {}
        void update() {}
    };

}
*/
//#endif


#endif
