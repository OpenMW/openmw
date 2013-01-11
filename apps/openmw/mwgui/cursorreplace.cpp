#include "cursorreplace.hpp"

#include <boost/filesystem.hpp>
#include <openengine/ogre/imagerotate.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <MyGUI.h>

using namespace MWGui;


#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceImageSetPointer.h"
#include "MyGUI_ImageBox.h"
#include "MyGUI_ResourceManager.h"



	ResourceImageSetPointerFix::ResourceImageSetPointerFix() :
		mImageSet(nullptr)
	{
	}

	ResourceImageSetPointerFix::~ResourceImageSetPointerFix()
	{
	}

	void ResourceImageSetPointerFix::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator info = _node->getElementEnumerator();
		while (info.next("Property"))
		{
			const std::string& key = info->findAttribute("key");
			const std::string& value = info->findAttribute("value");

			if (key == "Point")
				mPoint = IntPoint::parse(value);
			else if (key == "Size")
				mSize = IntSize::parse(value);
			else if (key == "Resource")
				mImageSet = ResourceManager::getInstance().getByName(value)->castType<ResourceImageSet>();
		}
	}

	void ResourceImageSetPointerFix::setImage(ImageBox* _image)
	{
		if (mImageSet != nullptr)
			_image->setItemResourceInfo(mImageSet->getIndexInfo(0, 0));
	}

	void ResourceImageSetPointerFix::setPosition(ImageBox* _image, const IntPoint& _point)
	{
		_image->setCoord(_point.left - mPoint.left, _point.top - mPoint.top, mSize.width, mSize.height);
	}

    ResourceImageSetPtr ResourceImageSetPointerFix:: getImageSet()
    {
        return mImageSet;
    }

    IntPoint ResourceImageSetPointerFix::getHotSpot()
    {
        return mPoint;
    }

    IntSize ResourceImageSetPointerFix::getSize()
    {
        return mSize;
    }

CursorReplace::CursorReplace()
{
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_vresize.png", 90);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize1.png", -45);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize2.png", 45);
}
