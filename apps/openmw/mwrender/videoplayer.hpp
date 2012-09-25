#ifndef MWRENDER_VIDEOPLAYER_H
#define MWRENDER_VIDEOPLAYER_H

//#ifdef OPENMW_USE_FFMPEG



#include <string>

#include <OgreDataStream.h>
#include <OgreTexture.h>

namespace Ogre
{
    class Rectangle2D;
    class SceneManager;
    class TextureUnitState;
}

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct SwsContext;
struct AVPacket;

namespace MWRender
{

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


        bool mEOF;

        // VIDEO
        AVCodecContext* mVideoCodecContext;
        AVCodec* mVideoCodec;
        int mVideoStreamId;
        AVFrame* mRawFrame;
        AVFrame* mRGBAFrame;
        SwsContext* mSwsContext;
        float mWantedFrameTime;
        float mDecodingTime;
        std::queue <AVPacket *> mVideoPacketQueue;


        bool readFrameAndQueue();
        bool saveFrame(AVPacket* frame);

        void decodeFrontFrame();

        void close();
        void deleteContext();


        void throwError(int error);
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
