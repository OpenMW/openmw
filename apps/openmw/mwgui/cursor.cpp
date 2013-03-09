#include "cursor.hpp"

#include <MyGUI_PointerManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_RotatingSkin.h>
#include <MyGUI_Gui.h>

#include <OgreMath.h>


namespace MWGui
{


    ResourceImageSetPointerFix::ResourceImageSetPointerFix()
        : mImageSet(NULL)
        , mRotation(0)
    {
    }

    ResourceImageSetPointerFix::~ResourceImageSetPointerFix()
    {
    }

    void ResourceImageSetPointerFix::deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version)
    {
        Base::deserialization(_node, _version);

        MyGUI::xml::ElementEnumerator info = _node->getElementEnumerator();
        while (info.next("Property"))
        {
            const std::string& key = info->findAttribute("key");
            const std::string& value = info->findAttribute("value");

            if (key == "Point")
                mPoint = MyGUI::IntPoint::parse(value);
            else if (key == "Size")
                mSize = MyGUI::IntSize::parse(value);
            else if (key == "Rotation")
                mRotation = MyGUI::utility::parseInt(value);
            else if (key == "Resource")
                mImageSet = MyGUI::ResourceManager::getInstance().getByName(value)->castType<MyGUI::ResourceImageSet>();
        }
    }

    int ResourceImageSetPointerFix::getRotation()
    {
        return mRotation;
    }

    void ResourceImageSetPointerFix::setImage(MyGUI::ImageBox* _image)
    {
        if (mImageSet != NULL)
            _image->setItemResourceInfo(mImageSet->getIndexInfo(0, 0));
    }

    void ResourceImageSetPointerFix::setPosition(MyGUI::ImageBox* _image, const MyGUI::IntPoint& _point)
    {
        _image->setCoord(_point.left - mPoint.left, _point.top - mPoint.top, mSize.width, mSize.height);
    }

    MyGUI::ResourceImageSetPtr ResourceImageSetPointerFix:: getImageSet()
    {
        return mImageSet;
    }

    MyGUI::IntPoint ResourceImageSetPointerFix::getHotSpot()
    {
        return mPoint;
    }

    MyGUI::IntSize ResourceImageSetPointerFix::getSize()
    {
        return mSize;
    }

    // ----------------------------------------------------------------------------------------

    Cursor::Cursor()
    {
        // hide mygui's pointer since we're rendering it ourselves (because mygui's pointer doesn't support rotation)
        MyGUI::PointerManager::getInstance().setVisible(false);

        MyGUI::PointerManager::getInstance().eventChangeMousePointer += MyGUI::newDelegate(this, &Cursor::onCursorChange);

        mWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::ImageBox>("RotatingSkin",0,0,0,0,MyGUI::Align::Default,"Pointer","");

        onCursorChange(MyGUI::PointerManager::getInstance().getDefaultPointer());
    }

    Cursor::~Cursor()
    {
    }

    void Cursor::onCursorChange(const std::string &name)
    {
        ResourceImageSetPointerFix* imgSetPtr = dynamic_cast<ResourceImageSetPointerFix*>(
                    MyGUI::PointerManager::getInstance().getByName(name));
        assert(imgSetPtr != NULL);

        MyGUI::ResourceImageSet* imgSet = imgSetPtr->getImageSet();

        std::string texture = imgSet->getIndexInfo(0,0).texture;

        mSize = imgSetPtr->getSize();
        mHotSpot = imgSetPtr->getHotSpot();

        int rotation = imgSetPtr->getRotation();

        mWidget->setImageTexture(texture);
        MyGUI::ISubWidget* main = mWidget->getSubWidgetMain();
        MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
        rotatingSubskin->setCenter(MyGUI::IntPoint(mSize.width/2,mSize.height/2));
        rotatingSubskin->setAngle(Ogre::Degree(rotation).valueRadians());
    }

    void Cursor::update()
    {
        MyGUI::IntPoint position = MyGUI::InputManager::getInstance().getMousePosition();

        mWidget->setPosition(position - mHotSpot);
        mWidget->setSize(mSize);
    }

    void Cursor::setVisible(bool visible)
    {
        mWidget->setVisible(visible);
    }

}
