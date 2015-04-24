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

        /// Adjust the coordinates of this video widget relative to its parent,
        /// based on the dimensions of the playing video.
        /// @param stretch Stretch the video to fill the whole screen? If false,
        ///                black bars may be added to fix the aspect ratio.
        void autoResize (bool stretch);

    private:
        std::auto_ptr<Video::VideoPlayer> mPlayer;
    };

}

#endif
