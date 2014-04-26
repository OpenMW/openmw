#include "videowidget.hpp"

namespace MWGui
{

VideoWidget::VideoWidget()
    : mAllowSkipping(true)
{
    eventKeyButtonPressed += MyGUI::newDelegate(this, &VideoWidget::onKeyPressed);

    setNeedKeyFocus(true);
}

void VideoWidget::playVideo(const std::string &video, bool allowSkipping)
{
    mAllowSkipping = allowSkipping;

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

void VideoWidget::onKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
    if (_key == MyGUI::KeyCode::Escape && mAllowSkipping)
        mPlayer.stopVideo();
}

bool VideoWidget::update()
{
    mPlayer.update();
    return mPlayer.isPlaying();
}

}
