#include "videowidget.hpp"

#include <extern/ogre-ffmpeg-videoplayer/videoplayer.hpp>

#include "../mwsound/movieaudiofactory.hpp"

namespace MWGui
{

VideoWidget::VideoWidget()
{
    mPlayer.reset(new Video::VideoPlayer());
    setNeedKeyFocus(true);
}

void VideoWidget::playVideo(const std::string &video)
{
    mPlayer->setAudioFactory(new MWSound::MovieAudioFactory());
    mPlayer->playVideo(video);

    setImageTexture(mPlayer->getTextureName());
}

int VideoWidget::getVideoWidth()
{
    return mPlayer->getVideoWidth();
}

int VideoWidget::getVideoHeight()
{
    return mPlayer->getVideoHeight();
}

bool VideoWidget::update()
{
    return mPlayer->update();
}

void VideoWidget::stop()
{
    mPlayer->close();
}

bool VideoWidget::hasAudioStream()
{
    return mPlayer->hasAudioStream();
}

}
