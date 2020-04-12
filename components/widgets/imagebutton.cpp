#include "imagebutton.hpp"

#include <MyGUI_RenderManager.h>

#include <components/debug/debuglog.hpp>

namespace Gui
{

    bool ImageButton::sDefaultNeedKeyFocus = true;

    ImageButton::ImageButton()
        : Base()
        , mMouseFocus(false)
        , mMousePress(false)
        , mKeyFocus(false)
        , mUseWholeTexture(true)
        , mTextureRect(MyGUI::IntCoord(0, 0, 0, 0))
    {
        setNeedKeyFocus(sDefaultNeedKeyFocus);
    }

    void ImageButton::setDefaultNeedKeyFocus(bool enabled)
    {
        sDefaultNeedKeyFocus = enabled;
    }

    void ImageButton::setTextureRect(MyGUI::IntCoord coord)
    {
        mTextureRect = coord;
        mUseWholeTexture = (coord == MyGUI::IntCoord(0, 0, 0, 0));
        updateImage();
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
        else if (_key == "TextureRect")
        {
            mTextureRect = MyGUI::IntCoord::parse(_value);
            mUseWholeTexture = (mTextureRect == MyGUI::IntCoord(0, 0, 0, 0));
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
        std::string textureName = mImageNormal;
        if (mMousePress)
            textureName = mImagePushed;
        else if (mMouseFocus || mKeyFocus)
            textureName = mImageHighlighted;

        if (!mUseWholeTexture)
        {
            int scale = 1.f;
            MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(textureName);
            if (texture && getHeight() != 0)
                scale = texture->getHeight() / getHeight();

            setImageTile(MyGUI::IntSize(mTextureRect.width * scale, mTextureRect.height * scale));
            MyGUI::IntCoord scaledSize(mTextureRect.left * scale, mTextureRect.top * scale, mTextureRect.width * scale, mTextureRect.height * scale);
            setImageCoord(scaledSize);
        }

        setImageTexture(textureName);
    }

    MyGUI::IntSize ImageButton::getRequestedSize()
    {
        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(mImageNormal);
        if (!texture)
        {
            Log(Debug::Error) << "ImageButton: can't find image " << mImageNormal;
            return MyGUI::IntSize(0,0);
        }

        if (mUseWholeTexture)
            return MyGUI::IntSize(texture->getWidth(), texture->getHeight());

        return MyGUI::IntSize(mTextureRect.width, mTextureRect.height);
    }

    void ImageButton::setImage(const std::string &image)
    {
        size_t extpos = image.find_last_of(".");
        std::string imageNoExt = image.substr(0, extpos);

        std::string ext = image.substr(extpos);

        mImageNormal = imageNoExt + "_idle" + ext;
        mImageHighlighted = imageNoExt + "_over" + ext;
        mImagePushed = imageNoExt + "_pressed" + ext;

        updateImage();
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
