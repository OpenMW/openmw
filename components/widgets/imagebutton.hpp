#ifndef OPENMW_COMPONENTS_WIDGETS_IMAGEBUTTON_H
#define OPENMW_COMPONENTS_WIDGETS_IMAGEBUTTON_H

#include <MyGUI_ImageBox.h>

namespace Gui
{

    /**
     * @brief allows using different image textures depending on the button state
     */
    class ImageButton final : public MyGUI::ImageBox
    {
        MYGUI_RTTI_DERIVED(ImageButton)

    public:
        MyGUI::IntSize getRequestedSize();

        ImageButton();

        static void setDefaultNeedKeyFocus(bool enabled);

        /// Set mImageNormal, mImageHighlighted and mImagePushed based on file convention (image_idle.ext, image_over.ext and image_pressed.ext)
        void setImage(const std::string& image);

        void setTextureRect(MyGUI::IntCoord coord);

    private:
        void updateImage();

        static bool sDefaultNeedKeyFocus;

    protected:
        void setPropertyOverride(const std::string& _key, const std::string& _value) override;
        void onMouseLostFocus(MyGUI::Widget* _new) override;
        void onMouseSetFocus(MyGUI::Widget* _old) override;
        void onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id) override;
        void onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id) override;
        void onKeySetFocus(MyGUI::Widget* _old) override;
        void onKeyLostFocus(MyGUI::Widget* _new) override;

        std::string mImageHighlighted;
        std::string mImageNormal;
        std::string mImagePushed;

        bool mMouseFocus;
        bool mMousePress;
        bool mKeyFocus;
        bool mUseWholeTexture;

        MyGUI::IntCoord mTextureRect;
    };

}

#endif
