#include "cursor.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_PointerManager.h>
#include <MyGUI_RotatingSkin.h>

namespace MWGui
{

    ResourceImageSetPointerFix::ResourceImageSetPointerFix()
        : mImageSet(nullptr)
        , mRotation(0)
    {
    }

    ResourceImageSetPointerFix::~ResourceImageSetPointerFix() {}

    void ResourceImageSetPointerFix::deserialization(MyGUI::xml::ElementPtr node, MyGUI::Version version)
    {
        Base::deserialization(node, version);

        MyGUI::xml::ElementEnumerator info = node->getElementEnumerator();
        while (info.next("Property"))
        {
            auto key = info->findAttribute("key");
            auto value = info->findAttribute("value");

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

    void ResourceImageSetPointerFix::setImage(MyGUI::ImageBox* image)
    {
        if (mImageSet != nullptr)
            image->setItemResourceInfo(mImageSet->getIndexInfo(0, 0));
    }

    void ResourceImageSetPointerFix::setPosition(MyGUI::ImageBox* image, const MyGUI::IntPoint& point)
    {
        image->setCoord(point.left - mPoint.left, point.top - mPoint.top, mSize.width, mSize.height);
    }

    MyGUI::ResourceImageSetPtr ResourceImageSetPointerFix::getImageSet()
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
