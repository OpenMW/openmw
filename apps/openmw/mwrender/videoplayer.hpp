#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <OgreMaterial.h>

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class Rectangle2D;
}

namespace MWRender
{
    struct VideoState;

    class VideoPlayer
    {
    public:
        VideoPlayer(Ogre::SceneManager* sceneMgr);
        ~VideoPlayer();

        void playVideo (const std::string& resourceName, bool allowSkipping);

        void update();

        void close();
        void stopVideo();

        bool isPlaying();

        void setResolution (int w, int h) { mWidth = w; mHeight = h; }


    private:
        VideoState* mState;

        bool mAllowSkipping;

        Ogre::SceneManager* mSceneMgr;
        Ogre::MaterialPtr mVideoMaterial;
        Ogre::Rectangle2D* mRectangle;
        Ogre::Rectangle2D* mBackgroundRectangle;
        Ogre::SceneNode* mNode;
        Ogre::SceneNode* mBackgroundNode;

        int mWidth;
        int mHeight;
    };
}

#endif
