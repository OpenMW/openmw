#include "videowidget.hpp"

namespace MWGui
{

VideoWidget::VideoWidget()
{
    setNeedKeyFocus(true);
}

void VideoWidget::playVideo(const std::string &video)
{
    mPlayer.playVideo(video);

    setImageTexture(mPlayer.getTextureName());
}

int VideoWidget::getVideoWidth()
{
    return mPlayer.getVideoWidth();
}

int VideoWidget::getVideoHeight()
{
    return mPlayer.getVideoHeight();
}

bool VideoWidget::update()
{
    mPlayer.update();
    return mPlayer.isPlaying();
}

void VideoWidget::stop()
{
    mPlayer.close();
}

}
