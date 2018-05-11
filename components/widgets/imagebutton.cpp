#include "imagebutton.hpp"

#include <MyGUI_RenderManager.h>

namespace Gui
{

    bool ImageButton::sDefaultNeedKeyFocus = true;

    ImageButton::ImageButton()
        : Base()
        , mMouseFocus(false)
        , mMousePress(false)
        , mKeyFocus(false)
    {
        setNeedKeyFocus(sDefaultNeedKeyFocus);
    }

    void ImageButton::setDefaultNeedKeyFocus(bool enabled)
    {
        sDefaultNeedKeyFocus = enabled;
    }

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
        mMouseFocus = true;
        updateImage();
        Base::onMouseSetFocus(_old);
    }

    void ImageButton::onMouseLostFocus(Widget* _new)
    {
        mMouseFocus = false;
        updateImage();
        Base::onMouseLostFocus(_new);
    }

    void ImageButton::onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id == MyGUI::MouseButton::Left)
        {
            mMousePress = true;
            updateImage();
        }
        Base::onMouseButtonPressed(_left, _top, _id);
    }

    void ImageButton::updateImage()
    {
        if (mMousePress)
            setImageTexture(mImagePushed);
        else if (mMouseFocus || mKeyFocus)
            setImageTexture(mImageHighlighted);
        else
            setImageTexture(mImageNormal);
    }

    MyGUI::IntSize ImageButton::getRequestedSize()
    {
        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(mImageNormal);
        if (!texture)
        {
            std::cerr << "ImageButton: can't find " << mImageNormal << std::endl;
            return MyGUI::IntSize(0,0);
        }
        return MyGUI::IntSize (texture->getWidth(), texture->getHeight());
    }

    void ImageButton::setImage(const std::string &image)
    {
        size_t extpos = image.find_last_of(".");
        std::string imageNoExt = image.substr(0, extpos);

        std::string ext = image.substr(extpos);

        mImageNormal = imageNoExt + "_idle" + ext;
        mImageHighlighted = imageNoExt + "_over" + ext;
        mImagePushed = imageNoExt + "_pressed" + ext;

        setImageTexture(mImageNormal);
    }

    void ImageButton::onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id == MyGUI::MouseButton::Left)
        {
            mMousePress = false;
            updateImage();
        }

        Base::onMouseButtonReleased(_left, _top, _id);
    }

    void ImageButton::onKeySetFocus(MyGUI::Widget *_old)
    {
        mKeyFocus = true;
        updateImage();
    }

    void ImageButton::onKeyLostFocus(MyGUI::Widget *_new)
    {
        mKeyFocus = false;
        updateImage();
    }
}
