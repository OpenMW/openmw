/*!
	@file
	@author		Albert Semenov
	@date		06/2009
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceManualPointer.h"
#include "MyGUI_StaticImage.h"
#include "MyGUI_CoordConverter.h"
#include "MyGUI_TextureUtility.h"

namespace MyGUI
{

	void ResourceManualPointer::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		IntCoord coord;

		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator info = _node->getElementEnumerator();
		while (info.next("Property"))
		{
			const std::string& key = info->findAttribute("key");
			const std::string& value = info->findAttribute("value");

			if (key == "Point") mPoint = IntPoint::parse(value);
			else if (key == "Size") mSize = IntSize::parse(value);
			else if (key == "Texture") mTexture = value;
			else if (key == "Coord") coord = IntCoord::parse(value);
		}

		mOffset = CoordConverter::convertTextureCoord(
			coord,
			texture_utility::getTextureSize(mTexture));
	}

	void ResourceManualPointer::setImage(StaticImage* _image)
	{
		_image->deleteAllItems();
		_image->_setTextureName(mTexture);
		_image->_setUVSet(mOffset);
	}

	void ResourceManualPointer::setPosition(StaticImage* _image, const IntPoint& _point)
	{
		_image->setCoord(_point.left - mPoint.left, _point.top - mPoint.top, mSize.width, mSize.height);
	}

} // namespace MyGUI
