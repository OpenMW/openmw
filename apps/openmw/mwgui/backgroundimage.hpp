#ifndef OPENMW_MWGUI_BACKGROUNDIMAGE_H
#define OPENMW_MWGUI_BACKGROUNDIMAGE_H

#include <MyGUI_ImageBox.h>

namespace MWGui
{

    /**
     * @brief A variant of MyGUI::ImageBox with aspect ratio correction using black bars
     */
    class BackgroundImage final : public MyGUI::ImageBox
    {
    MYGUI_RTTI_DERIVED(BackgroundImage)

    public:
        BackgroundImage() : mChild(nullptr), mAspect(0) {}

        /**
         * @param fixedRatio Use a fixed ratio of 4:3, regardless of the image dimensions
         * @param stretch Stretch to fill the whole screen, or add black bars?
         */
        void setBackgroundImage (const std::string& image, bool fixedRatio=true, bool stretch=true);

        void setSize (const MyGUI::IntSize &_value) final;
        void setCoord (const MyGUI::IntCoord &_value) final;

    private:
        MyGUI::ImageBox* mChild;
        double mAspect;

        void adjustSize();
    };

}

#endif
