#include "videowidget.hpp"

#include <algorithm>
#include <exception>
#include <utility>

#include <MyGUI_ISubWidgetRect.h>
#include <MyGUI_ITexture.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Types.h>

#include <osg/ref_ptr>

#include <extern/osg-ffmpeg-videoplayer/videoplayer.hpp>

#include <components/debug/debuglog.hpp>
#include <components/files/istreamptr.hpp>
#include <components/myguiplatform/myguitexture.hpp>
#include <components/vfs/manager.hpp>

#include "../mwsound/movieaudiofactory.hpp"

namespace osg
{
    class Texture2D;
}

namespace MWGui
{

    VideoWidget::VideoWidget()
        : mVFS(nullptr)
    {
        mPlayer = std::make_unique<Video::VideoPlayer>();
        setNeedKeyFocus(true);
    }

    VideoWidget::~VideoWidget() = default;

    void VideoWidget::setVFS(const VFS::Manager* vfs)
    {
        mVFS = vfs;
    }

    void VideoWidget::playVideo(const std::string& video)
    {
        mPlayer->setAudioFactory(new MWSound::MovieAudioFactory());

        Files::IStreamPtr videoStream;
        try
        {
            videoStream = mVFS->get(video);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Failed to open video: " << e.what();
            return;
        }

        mPlayer->playVideo(std::move(videoStream), video);

        osg::ref_ptr<osg::Texture2D> texture = mPlayer->getVideoTexture();
        if (!texture)
            return;

        mTexture = std::make_unique<osgMyGUI::OSGTexture>(texture);

        setRenderItemTexture(mTexture.get());
        getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 1.f, 1.f, 0.f));
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

    void VideoWidget::pause()
    {
        mPlayer->pause();
    }

    void VideoWidget::resume()
    {
        mPlayer->play();
    }

    bool VideoWidget::isPaused() const
    {
        return mPlayer->isPaused();
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
            double imageaspect = static_cast<double>(getVideoWidth()) / getVideoHeight();

            int leftPadding = std::max(0, static_cast<int>(screenSize.width - screenSize.height * imageaspect) / 2);
            int topPadding = std::max(0, static_cast<int>(screenSize.height - screenSize.width / imageaspect) / 2);

            setCoord(leftPadding, topPadding, screenSize.width - leftPadding * 2, screenSize.height - topPadding * 2);
        }
        else
            setCoord(0, 0, screenSize.width, screenSize.height);
    }

}
