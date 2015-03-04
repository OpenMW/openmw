#include "imagebutton.hpp"

#include <MyGUI_RenderManager.h>

namespace Gui
{

    void ImageButton::setPropertyOverride(const std::string &_key, const std::string &_value)
    {
        if (_key == "ImageHighlighted")
            mImageHighlighted = _value;
        else if (_key == "ImagePushed")
            mImagePushed = _value;
        else if (_key == "ImageNormal")
        {
            if (mImageNormal == "")
            {
                setImageTexture(_value);
            }
            mImageNormal = _value;
        }
        else
            ImageBox::setPropertyOverride(_key, _value);
    }
    void ImageButton::onMouseSetFocus(Widget* _old)
    {
        setImageTexture(mImageHighlighted);
        ImageBox::onMouseSetFocus(_old);
    }

    void ImageButton::onMouseLostFocus(Widget* _new)
    {
        setImageTexture(mImageNormal);
        ImageBox::onMouseLostFocus(_new);
    }

    void ImageButton::onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id == MyGUI::MouseButton::Left)
            setImageTexture(mImagePushed);

        ImageBox::onMouseButtonPressed(_left, _top, _id);
    }

    MyGUI::IntSize ImageButton::getRequestedSize(bool logError)
    {
        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(mImageNormal);
        if (!texture)
        {
            if (logError)
                std::cerr << "ImageButton: can't find " << mImageNormal << std::endl;
            return MyGUI::IntSize(0,0);
        }
        return MyGUI::IntSize (texture->getWidth(), texture->getHeight());
    }

    void ImageButton::onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id == MyGUI::MouseButton::Left)
            setImageTexture(mImageHighlighted);

        ImageBox::onMouseButtonReleased(_left, _top, _id);
    }
}
