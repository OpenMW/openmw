#include "cursor.hpp"

#include <MyGUI_PointerManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RotatingSkin.h>
#include <MyGUI_Gui.h>

namespace MWGui
{


    ResourceImageSetPointerFix::ResourceImageSetPointerFix()
        : mImageSet(nullptr)
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
        if (mImageSet != nullptr)
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

}
