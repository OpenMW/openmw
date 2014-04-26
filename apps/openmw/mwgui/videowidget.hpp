#ifndef OPENMW_MWGUI_VIDEOWIDGET_H
#define OPENMW_MWGUI_VIDEOWIDGET_H

#include <MyGUI_ImageBox.h>

#include "../mwrender/videoplayer.hpp"

namespace MWGui
{

    /**
     * Widget that plays a video. Can be skipped by pressing Esc.
     */
    class VideoWidget : public MyGUI::ImageBox
    {
    public:
        MYGUI_RTTI_DERIVED(VideoWidget)

        VideoWidget();

        void playVideo (const std::string& video, bool allowSkipping);

        int getVideoWidth();
        int getVideoHeight();

        /// @return Is the video still playing?
        bool update();

    private:
        bool mAllowSkipping;

        MWRender::VideoPlayer mPlayer;

        void onKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char);
    };

}

#endif
