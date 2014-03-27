#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <string>

namespace MWRender
{
    struct VideoState;

    /**
     * @brief Plays a video on an Ogre texture.
     */
    class VideoPlayer
    {
    public:
        VideoPlayer();
        ~VideoPlayer();

        void playVideo (const std::string& resourceName);

        void update();

        void close();
        void stopVideo();

        bool isPlaying();

        std::string getTextureName();
        int getVideoWidth();
        int getVideoHeight();


    private:
        VideoState* mState;

        int mWidth;
        int mHeight;
    };
}

#endif
