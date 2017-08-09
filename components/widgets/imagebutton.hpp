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

        /// Set mImageNormal, mImageHighlighted and mImagePushed based on file convention (image_idle.ext, image_over.ext and image_pressed.ext)
        void setImage(const std::string& image);

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
        virtual void onMouseLostFocus(MyGUI::Widget* _new);
        virtual void onMouseSetFocus(MyGUI::Widget* _old);
        virtual void onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id);
        virtual void onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id);

        std::string mImageHighlighted;
        std::string mImageNormal;
        std::string mImagePushed;
    };

}

#endif
