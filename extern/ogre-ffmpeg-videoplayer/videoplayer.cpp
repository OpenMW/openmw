#include "videoplayer.hpp"

#include "videostate.hpp"

namespace Video
{

VideoPlayer::VideoPlayer()
    : mState(NULL)
{

}

VideoPlayer::~VideoPlayer()
{
    if(mState)
        close();
}

void VideoPlayer::setAudioFactory(MovieAudioFactory *factory)
{
    mAudioFactory.reset(factory);
}

void VideoPlayer::playVideo(const std::string &resourceName)
{
    if(mState)
        close();

    try {
        mState = new VideoState;
        mState->setAudioFactory(mAudioFactory.get());
        mState->init(resourceName);
    }
    catch(std::exception& e) {
        std::cerr<< "Failed to play video: "<<e.what() <<std::endl;
        close();
    }
}

void VideoPlayer::update ()
{
    if(mState)
    {
        if(!mState->update())
            close();
    }
}

std::string VideoPlayer::getTextureName()
{
    std::string name;
    if (mState)
        name = mState->mTexture->getName();
    return name;
}

int VideoPlayer::getVideoWidth()
{
    int width=0;
    if (mState)
        width = mState->mTexture->getWidth();
    return width;
}

int VideoPlayer::getVideoHeight()
{
    int height=0;
    if (mState)
        height = mState->mTexture->getHeight();
    return height;
}

void VideoPlayer::close()
{
    if(mState)
    {
        mState->deinit();

        delete mState;
        mState = NULL;
    }
}

bool VideoPlayer::isPlaying ()
{
    return mState != NULL;
}

bool VideoPlayer::hasAudioStream()
{
    return mState && mState->audio_st != NULL;
}

}
