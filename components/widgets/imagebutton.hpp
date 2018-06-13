#ifndef OPENMW_COMPONENTS_WIDGETS_IMAGEBUTTON_H
#define OPENMW_COMPONENTS_WIDGETS_IMAGEBUTTON_H

#include <MyGUI_ImageBox.h>

namespace Gui
{

    /**
     * @brief allows using different image textures depending on the button state
     */
    class ImageButton : public MyGUI::ImageBox
    {
        MYGUI_RTTI_DERIVED(ImageButton)

    public:
        MyGUI::IntSize getRequestedSize();

        ImageButton();

        static void setDefaultNeedKeyFocus(bool enabled);

        /// Set mImageNormal, mImageHighlighted and mImagePushed based on file convention (image_idle.ext, image_over.ext and image_pressed.ext)
        void setImage(const std::string& image);

    private:
        void updateImage();

        static bool sDefaultNeedKeyFocus;

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
        virtual void onMouseLostFocus(MyGUI::Widget* _new);
        virtual void onMouseSetFocus(MyGUI::Widget* _old);
        virtual void onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id);
        virtual void onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id);
        virtual void onKeySetFocus(MyGUI::Widget* _old);
        virtual void onKeyLostFocus(MyGUI::Widget* _new);

        std::string mImageHighlighted;
        std::string mImageNormal;
        std::string mImagePushed;

        bool mMouseFocus;
        bool mMousePress;
        bool mKeyFocus;
    };

}

#endif
