#ifndef OPENMW_MWGUI_VIDEOWIDGET_H
#define OPENMW_MWGUI_VIDEOWIDGET_H

#include <MyGUI_ImageBox.h>

#include "../mwrender/videoplayer.hpp"

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

        /// Stop video and free resources (done automatically on destruction)
        void stop();

    private:
        MWRender::VideoPlayer mPlayer;
    };

}

#endif
