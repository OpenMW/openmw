#ifndef OPENMW_MWGUI_VIDEOWIDGET_H
#define OPENMW_MWGUI_VIDEOWIDGET_H

#include <MyGUI_ImageBox.h>

namespace Video
{
    class VideoPlayer;
}

namespace MWGui
{

    /**
     * Widget that plays a video.
     */
    class VideoWidget : public MyGUI::ImageBox
    {
    public:
        MYGUI_RTTI_DERIVED(VideoWidget)

        VideoWidget();

        void playVideo (const std::string& video);

        int getVideoWidth();
        int getVideoHeight();

        /// @return Is the video still playing?
        bool update();

        /// Return true if a video is currently playing and it has an audio stream.
        bool hasAudioStream();

        /// Stop video and free resources (done automatically on destruction)
        void stop();

    private:
        std::auto_ptr<Video::VideoPlayer> mPlayer;
    };

}

#endif
