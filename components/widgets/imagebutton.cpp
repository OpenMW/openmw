#include "imagebutton.hpp"

#include <cmath>

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

    void ImageButton::setPropertyOverride(std::string_view key, std::string_view value)
    {
        if (key == "ImageHighlighted")
            mImageHighlighted = value;
        else if (key == "ImagePushed")
            mImagePushed = value;
        else if (key == "ImageNormal")
        {
            if (mImageNormal.empty())
            {
                setImageTexture(value);
            }
            mImageNormal = value;
        }
        else if (key == "TextureRect")
        {
            mTextureRect = MyGUI::IntCoord::parse(value);
            mUseWholeTexture = (mTextureRect == MyGUI::IntCoord(0, 0, 0, 0));
        }
        else
            ImageBox::setPropertyOverride(key, value);
    }

    void ImageButton::onMouseSetFocus(MyGUI::Widget* oldWidget)
    {
        mMouseFocus = true;
        updateImage();
        Base::onMouseSetFocus(oldWidget);
    }

    void ImageButton::onMouseLostFocus(MyGUI::Widget* newWidget)
    {
        mMouseFocus = false;
        updateImage();
        Base::onMouseLostFocus(newWidget);
    }

    void ImageButton::onMouseButtonPressed(int left, int top, MyGUI::MouseButton id)
    {
        if (id == MyGUI::MouseButton::Left)
        {
            mMousePress = true;
            updateImage();
        }
        Base::onMouseButtonPressed(left, top, id);
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
            float scale = 1.f;
            MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(textureName);
            if (texture && getHeight() != 0)
                scale = static_cast<float>(texture->getHeight()) / getHeight();

            const int width = static_cast<int>(std::round(mTextureRect.width * scale));
            const int height = static_cast<int>(std::round(mTextureRect.height * scale));
            setImageTile(MyGUI::IntSize(width, height));
            MyGUI::IntCoord scaledSize(static_cast<int>(std::round(mTextureRect.left * scale)),
                static_cast<int>(std::round(mTextureRect.top * scale)), width, height);
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
            return MyGUI::IntSize(0, 0);
        }

        if (mUseWholeTexture)
            return MyGUI::IntSize(texture->getWidth(), texture->getHeight());

        return MyGUI::IntSize(mTextureRect.width, mTextureRect.height);
    }

    void ImageButton::setImage(const std::string& image)
    {
        size_t extpos = image.find_last_of('.');
        std::string imageNoExt = image.substr(0, extpos);

        std::string ext = image.substr(extpos);

        mImageNormal = imageNoExt + "_idle" + ext;
        mImageHighlighted = imageNoExt + "_over" + ext;
        mImagePushed = imageNoExt + "_pressed" + ext;

        updateImage();
    }

    void ImageButton::onMouseButtonReleased(int left, int top, MyGUI::MouseButton id)
    {
        if (id == MyGUI::MouseButton::Left)
        {
            mMousePress = false;
            updateImage();
        }

        Base::onMouseButtonReleased(left, top, id);
    }

    void ImageButton::onKeySetFocus(MyGUI::Widget* /*newWidget*/)
    {
        mKeyFocus = true;
        updateImage();
    }

    void ImageButton::onKeyLostFocus(MyGUI::Widget* /*oldWidget*/)
    {
        mKeyFocus = false;
        updateImage();
    }
}
