#include "videowidget.hpp"

#include <extern/ogre-ffmpeg-videoplayer/videoplayer.hpp>

#include <MyGUI_RenderManager.h>

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

void VideoWidget::autoResize(bool stretch)
{
    MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
    if (getParent())
        screenSize = getParent()->getSize();

    if (getVideoHeight() > 0 && !stretch)
    {
        double imageaspect = static_cast<double>(getVideoWidth())/getVideoHeight();

        int leftPadding = std::max(0, static_cast<int>(screenSize.width - screenSize.height * imageaspect) / 2);
        int topPadding = std::max(0, static_cast<int>(screenSize.height - screenSize.width / imageaspect) / 2);

        setCoord(leftPadding, topPadding,
                               screenSize.width - leftPadding*2, screenSize.height - topPadding*2);
    }
    else
        setCoord(0,0,screenSize.width,screenSize.height);
}

}
